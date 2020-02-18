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

namespace Opc.Ua.MTConnect
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
        /// The identifier for the ActuatorStateTypeEnum DataType.
        /// </summary>
        public const uint ActuatorStateTypeEnum = 9963;

        /// <summary>
        /// The identifier for the AlarmStateTypeEnum DataType.
        /// </summary>
        public const uint AlarmStateTypeEnum = 9965;

        /// <summary>
        /// The identifier for the AssetDescriptionType DataType.
        /// </summary>
        public const uint AssetDescriptionType = 9967;

        /// <summary>
        /// The identifier for the AssetAttrDataType DataType.
        /// </summary>
        public const uint AssetAttrDataType = 9968;

        /// <summary>
        /// The identifier for the AssetBufferSizeDataType DataType.
        /// </summary>
        public const uint AssetBufferSizeDataType = 9969;

        /// <summary>
        /// The identifier for the AssetIdDataType DataType.
        /// </summary>
        public const uint AssetIdDataType = 9970;

        /// <summary>
        /// The identifier for the AvailabilityTypeEnum DataType.
        /// </summary>
        public const uint AvailabilityTypeEnum = 9971;

        /// <summary>
        /// The identifier for the AxesCouplingTypeEnum DataType.
        /// </summary>
        public const uint AxesCouplingTypeEnum = 9973;

        /// <summary>
        /// The identifier for the AxesListValueDataType DataType.
        /// </summary>
        public const uint AxesListValueDataType = 9975;

        /// <summary>
        /// The identifier for the BufferSizeDataType DataType.
        /// </summary>
        public const uint BufferSizeDataType = 9976;

        /// <summary>
        /// The identifier for the CalibrationDateDataType DataType.
        /// </summary>
        public const uint CalibrationDateDataType = 9977;

        /// <summary>
        /// The identifier for the CalibrationInitialsDataType DataType.
        /// </summary>
        public const uint CalibrationInitialsDataType = 9978;

        /// <summary>
        /// The identifier for the CategoryTypeEnum DataType.
        /// </summary>
        public const uint CategoryTypeEnum = 9979;

        /// <summary>
        /// The identifier for the ChannelNumberDataType DataType.
        /// </summary>
        public const uint ChannelNumberDataType = 9981;

        /// <summary>
        /// The identifier for the ClampStateTypeEnum DataType.
        /// </summary>
        public const uint ClampStateTypeEnum = 9982;

        /// <summary>
        /// The identifier for the ConditionDescriptionDataType DataType.
        /// </summary>
        public const uint ConditionDescriptionDataType = 9984;

        /// <summary>
        /// The identifier for the ControllerModeTypeEnum DataType.
        /// </summary>
        public const uint ControllerModeTypeEnum = 9985;

        /// <summary>
        /// The identifier for the CoordinateSystemTypeEnum DataType.
        /// </summary>
        public const uint CoordinateSystemTypeEnum = 9987;

        /// <summary>
        /// The identifier for the IDDataType DataType.
        /// </summary>
        public const uint IDDataType = 9989;

        /// <summary>
        /// The identifier for the CodeDataType DataType.
        /// </summary>
        public const uint CodeDataType = 9990;

        /// <summary>
        /// The identifier for the ComponentIdDataType DataType.
        /// </summary>
        public const uint ComponentIdDataType = 9991;

        /// <summary>
        /// The identifier for the ConnectionCodeMachineSideDataType DataType.
        /// </summary>
        public const uint ConnectionCodeMachineSideDataType = 9992;

        /// <summary>
        /// The identifier for the CountValueDataType DataType.
        /// </summary>
        public const uint CountValueDataType = 9993;

        /// <summary>
        /// The identifier for the CreationTimeDataType DataType.
        /// </summary>
        public const uint CreationTimeDataType = 9994;

        /// <summary>
        /// The identifier for the CutterStatusValueTypeEnum DataType.
        /// </summary>
        public const uint CutterStatusValueTypeEnum = 9995;

        /// <summary>
        /// The identifier for the DataItemEnumDataType DataType.
        /// </summary>
        public const uint DataItemEnumDataType = 9997;

        /// <summary>
        /// The identifier for the DataItemEnumTypeEnum DataType.
        /// </summary>
        public const uint DataItemEnumTypeEnum = 9998;

        /// <summary>
        /// The identifier for the DataItemIdDataType DataType.
        /// </summary>
        public const uint DataItemIdDataType = 10000;

        /// <summary>
        /// The identifier for the DataItemStatisticsDataType DataType.
        /// </summary>
        public const uint DataItemStatisticsDataType = 10001;

        /// <summary>
        /// The identifier for the DataItemStatisticsTypeEnum DataType.
        /// </summary>
        public const uint DataItemStatisticsTypeEnum = 10002;

        /// <summary>
        /// The identifier for the DataItemSubEnumTypeEnum DataType.
        /// </summary>
        public const uint DataItemSubEnumTypeEnum = 10004;

        /// <summary>
        /// The identifier for the DefinitionFormatTypeEnum DataType.
        /// </summary>
        public const uint DefinitionFormatTypeEnum = 10006;

        /// <summary>
        /// The identifier for the DecibelValueDataType DataType.
        /// </summary>
        public const uint DecibelValueDataType = 10008;

        /// <summary>
        /// The identifier for the DescriptionTextDataType DataType.
        /// </summary>
        public const uint DescriptionTextDataType = 10009;

        /// <summary>
        /// The identifier for the DirectionTypeEnum DataType.
        /// </summary>
        public const uint DirectionTypeEnum = 10010;

        /// <summary>
        /// The identifier for the DoorStateTypeEnum DataType.
        /// </summary>
        public const uint DoorStateTypeEnum = 10012;

        /// <summary>
        /// The identifier for the DurationTimeDataType DataType.
        /// </summary>
        public const uint DurationTimeDataType = 10014;

        /// <summary>
        /// The identifier for the DurationValueDataType DataType.
        /// </summary>
        public const uint DurationValueDataType = 10015;

        /// <summary>
        /// The identifier for the EdgeCountDataType DataType.
        /// </summary>
        public const uint EdgeCountDataType = 10016;

        /// <summary>
        /// The identifier for the EmergencyStopTypeEnum DataType.
        /// </summary>
        public const uint EmergencyStopTypeEnum = 10017;

        /// <summary>
        /// The identifier for the EnergyValueDataType DataType.
        /// </summary>
        public const uint EnergyValueDataType = 10019;

        /// <summary>
        /// The identifier for the ExecutionTypeEnum DataType.
        /// </summary>
        public const uint ExecutionTypeEnum = 10020;

        /// <summary>
        /// The identifier for the FeedrateValueDataType DataType.
        /// </summary>
        public const uint FeedrateValueDataType = 10022;

        /// <summary>
        /// The identifier for the FirmwareVersionDataType DataType.
        /// </summary>
        public const uint FirmwareVersionDataType = 10023;

        /// <summary>
        /// The identifier for the ForceValueDataType DataType.
        /// </summary>
        public const uint ForceValueDataType = 10024;

        /// <summary>
        /// The identifier for the GradeDataType DataType.
        /// </summary>
        public const uint GradeDataType = 10025;

        /// <summary>
        /// The identifier for the IndexRangeDataType DataType.
        /// </summary>
        public const uint IndexRangeDataType = 10026;

        /// <summary>
        /// The identifier for the InstanceIdDataType DataType.
        /// </summary>
        public const uint InstanceIdDataType = 10027;

        /// <summary>
        /// The identifier for the ItemIdDataType DataType.
        /// </summary>
        public const uint ItemIdDataType = 10028;

        /// <summary>
        /// The identifier for the InterfaceStateTypeEnum DataType.
        /// </summary>
        public const uint InterfaceStateTypeEnum = 10029;

        /// <summary>
        /// The identifier for the ItemSourceDataType DataType.
        /// </summary>
        public const uint ItemSourceDataType = 10031;

        /// <summary>
        /// The identifier for the LocationSizeDataType DataType.
        /// </summary>
        public const uint LocationSizeDataType = 10032;

        /// <summary>
        /// The identifier for the LocationValueDataType DataType.
        /// </summary>
        public const uint LocationValueDataType = 10033;

        /// <summary>
        /// The identifier for the LocationsTypeEnum DataType.
        /// </summary>
        public const uint LocationsTypeEnum = 10034;

        /// <summary>
        /// The identifier for the LocusDataType DataType.
        /// </summary>
        public const uint LocusDataType = 10036;

        /// <summary>
        /// The identifier for the ManufacturersDataType DataType.
        /// </summary>
        public const uint ManufacturersDataType = 10037;

        /// <summary>
        /// The identifier for the MassValueDataType DataType.
        /// </summary>
        public const uint MassValueDataType = 10038;

        /// <summary>
        /// The identifier for the MaximumCountDataType DataType.
        /// </summary>
        public const uint MaximumCountDataType = 10039;

        /// <summary>
        /// The identifier for the MaximumDataType DataType.
        /// </summary>
        public const uint MaximumDataType = 10040;

        /// <summary>
        /// The identifier for the MeasurementValueDataType DataType.
        /// </summary>
        public const uint MeasurementValueDataType = 10041;

        /// <summary>
        /// The identifier for the MinimumDataType DataType.
        /// </summary>
        public const uint MinimumDataType = 10042;

        /// <summary>
        /// The identifier for the ModelDataType DataType.
        /// </summary>
        public const uint ModelDataType = 10043;

        /// <summary>
        /// The identifier for the NameDataType DataType.
        /// </summary>
        public const uint NameDataType = 10044;

        /// <summary>
        /// The identifier for the NativeCodeDataType DataType.
        /// </summary>
        public const uint NativeCodeDataType = 10045;

        /// <summary>
        /// The identifier for the NativeNotifcationCodeDataType DataType.
        /// </summary>
        public const uint NativeNotifcationCodeDataType = 10046;

        /// <summary>
        /// The identifier for the NativeScaleDataType DataType.
        /// </summary>
        public const uint NativeScaleDataType = 10047;

        /// <summary>
        /// The identifier for the NativeSeverityDataType DataType.
        /// </summary>
        public const uint NativeSeverityDataType = 10048;

        /// <summary>
        /// The identifier for the NextCalibrationDateDataType DataType.
        /// </summary>
        public const uint NextCalibrationDateDataType = 10049;

        /// <summary>
        /// The identifier for the NominalDataType DataType.
        /// </summary>
        public const uint NominalDataType = 10050;

        /// <summary>
        /// The identifier for the NotificationCodeTypeEnum DataType.
        /// </summary>
        public const uint NotificationCodeTypeEnum = 10051;

        /// <summary>
        /// The identifier for the NotificationDescriptionDataType DataType.
        /// </summary>
        public const uint NotificationDescriptionDataType = 10053;

        /// <summary>
        /// The identifier for the OccurrenceTimeDataType DataType.
        /// </summary>
        public const uint OccurrenceTimeDataType = 10054;

        /// <summary>
        /// The identifier for the OverlapDataType DataType.
        /// </summary>
        public const uint OverlapDataType = 10055;

        /// <summary>
        /// The identifier for the PathModeTypeEnum DataType.
        /// </summary>
        public const uint PathModeTypeEnum = 10056;

        /// <summary>
        /// The identifier for the PowerStateTypeEnum DataType.
        /// </summary>
        public const uint PowerStateTypeEnum = 10058;

        /// <summary>
        /// The identifier for the ProgramToolNumberDataType DataType.
        /// </summary>
        public const uint ProgramToolNumberDataType = 10060;

        /// <summary>
        /// The identifier for the QualifierTypeEnum DataType.
        /// </summary>
        public const uint QualifierTypeEnum = 10061;

        /// <summary>
        /// The identifier for the RateDataType DataType.
        /// </summary>
        public const uint RateDataType = 10063;

        /// <summary>
        /// The identifier for the ReconditionCountValueDataType DataType.
        /// </summary>
        public const uint ReconditionCountValueDataType = 10064;

        /// <summary>
        /// The identifier for the RotaryModeTypeEnum DataType.
        /// </summary>
        public const uint RotaryModeTypeEnum = 10065;

        /// <summary>
        /// The identifier for the SampleRateDataType DataType.
        /// </summary>
        public const uint SampleRateDataType = 10067;

        /// <summary>
        /// The identifier for the SeverityTypeEnum DataType.
        /// </summary>
        public const uint SeverityTypeEnum = 10068;

        /// <summary>
        /// The identifier for the SignificantDigitsValueDataType DataType.
        /// </summary>
        public const uint SignificantDigitsValueDataType = 10070;

        /// <summary>
        /// The identifier for the StationDataType DataType.
        /// </summary>
        public const uint StationDataType = 10071;

        /// <summary>
        /// The identifier for the SenderDataType DataType.
        /// </summary>
        public const uint SenderDataType = 10072;

        /// <summary>
        /// The identifier for the SequenceDataType DataType.
        /// </summary>
        public const uint SequenceDataType = 10073;

        /// <summary>
        /// The identifier for the SerialNumberDataType DataType.
        /// </summary>
        public const uint SerialNumberDataType = 10074;

        /// <summary>
        /// The identifier for the SpeedDataType DataType.
        /// </summary>
        public const uint SpeedDataType = 10075;

        /// <summary>
        /// The identifier for the TestIndicatorDataType DataType.
        /// </summary>
        public const uint TestIndicatorDataType = 10076;

        /// <summary>
        /// The identifier for the ToolEventValueDataType DataType.
        /// </summary>
        public const uint ToolEventValueDataType = 10077;

        /// <summary>
        /// The identifier for the ToolGroupDataType DataType.
        /// </summary>
        public const uint ToolGroupDataType = 10078;

        /// <summary>
        /// The identifier for the ToolIdDataType DataType.
        /// </summary>
        public const uint ToolIdDataType = 10079;

        /// <summary>
        /// The identifier for the ToolLifeDirectionTypeEnum DataType.
        /// </summary>
        public const uint ToolLifeDirectionTypeEnum = 10080;

        /// <summary>
        /// The identifier for the ToolLifeTypeEnum DataType.
        /// </summary>
        public const uint ToolLifeTypeEnum = 10082;

        /// <summary>
        /// The identifier for the ToolLifeValueDataType DataType.
        /// </summary>
        public const uint ToolLifeValueDataType = 10084;

        /// <summary>
        /// The identifier for the UnitsExtDataType DataType.
        /// </summary>
        public const uint UnitsExtDataType = 10085;

        /// <summary>
        /// The identifier for the VersionDataType DataType.
        /// </summary>
        public const uint VersionDataType = 10086;

        /// <summary>
        /// The identifier for the VibrationValueDataType DataType.
        /// </summary>
        public const uint VibrationValueDataType = 10087;
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
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_Open Method.
        /// </summary>
        public const uint OpcUaMTConnectNamespaceMetadata_NamespaceFile_Open = 15015;

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_Close Method.
        /// </summary>
        public const uint OpcUaMTConnectNamespaceMetadata_NamespaceFile_Close = 15018;

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_Read Method.
        /// </summary>
        public const uint OpcUaMTConnectNamespaceMetadata_NamespaceFile_Read = 15020;

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_Write Method.
        /// </summary>
        public const uint OpcUaMTConnectNamespaceMetadata_NamespaceFile_Write = 15023;

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_GetPosition Method.
        /// </summary>
        public const uint OpcUaMTConnectNamespaceMetadata_NamespaceFile_GetPosition = 15025;

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_SetPosition Method.
        /// </summary>
        public const uint OpcUaMTConnectNamespaceMetadata_NamespaceFile_SetPosition = 15028;
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
        /// The identifier for the OpcUaMTConnectNamespaceMetadata Object.
        /// </summary>
        public const uint OpcUaMTConnectNamespaceMetadata = 15001;

        /// <summary>
        /// The identifier for the MTDevices Object.
        /// </summary>
        public const uint MTDevices = 10110;

        /// <summary>
        /// The identifier for the MTDeviceType_DataItems Object.
        /// </summary>
        public const uint MTDeviceType_DataItems = 6626;

        /// <summary>
        /// The identifier for the MTDeviceType_Components Object.
        /// </summary>
        public const uint MTDeviceType_Components = 6627;

        /// <summary>
        /// The identifier for the MTDeviceType_Conditions Object.
        /// </summary>
        public const uint MTDeviceType_Conditions = 6628;

        /// <summary>
        /// The identifier for the MTConditionType_LimitState Object.
        /// </summary>
        public const uint MTConditionType_LimitState = 7235;

        /// <summary>
        /// The identifier for the CuttingItemType_ItemLife Object.
        /// </summary>
        public const uint CuttingItemType_ItemLife = 9947;

        /// <summary>
        /// The identifier for the CuttingItemType_Measurements Object.
        /// </summary>
        public const uint CuttingItemType_Measurements = 9948;

        /// <summary>
        /// The identifier for the DataItemEnumDataType_Encoding_DefaultBinary Object.
        /// </summary>
        public const uint DataItemEnumDataType_Encoding_DefaultBinary = 10099;

        /// <summary>
        /// The identifier for the DataItemStatisticsDataType_Encoding_DefaultBinary Object.
        /// </summary>
        public const uint DataItemStatisticsDataType_Encoding_DefaultBinary = 10100;

        /// <summary>
        /// The identifier for the DataItemEnumDataType_Encoding_DefaultXml Object.
        /// </summary>
        public const uint DataItemEnumDataType_Encoding_DefaultXml = 10088;

        /// <summary>
        /// The identifier for the DataItemStatisticsDataType_Encoding_DefaultXml Object.
        /// </summary>
        public const uint DataItemStatisticsDataType_Encoding_DefaultXml = 10089;

        /// <summary>
        /// The identifier for the DataItemEnumDataType_Encoding_DefaultJson Object.
        /// </summary>
        public const uint DataItemEnumDataType_Encoding_DefaultJson = 10200;

        /// <summary>
        /// The identifier for the DataItemStatisticsDataType_Encoding_DefaultJson Object.
        /// </summary>
        public const uint DataItemStatisticsDataType_Encoding_DefaultJson = 10201;
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
        /// The identifier for the MTDevicesType ObjectType.
        /// </summary>
        public const uint MTDevicesType = 6614;

        /// <summary>
        /// The identifier for the MTDeviceType ObjectType.
        /// </summary>
        public const uint MTDeviceType = 6618;

        /// <summary>
        /// The identifier for the MTComponentType ObjectType.
        /// </summary>
        public const uint MTComponentType = 6629;

        /// <summary>
        /// The identifier for the MTAxesType ObjectType.
        /// </summary>
        public const uint MTAxesType = 6642;

        /// <summary>
        /// The identifier for the MTControllerType ObjectType.
        /// </summary>
        public const uint MTControllerType = 6659;

        /// <summary>
        /// The identifier for the MTDoorType ObjectType.
        /// </summary>
        public const uint MTDoorType = 6670;

        /// <summary>
        /// The identifier for the MTActuatorType ObjectType.
        /// </summary>
        public const uint MTActuatorType = 6682;

        /// <summary>
        /// The identifier for the MTConditionType ObjectType.
        /// </summary>
        public const uint MTConditionType = 7188;

        /// <summary>
        /// The identifier for the AccelerationConditionType ObjectType.
        /// </summary>
        public const uint AccelerationConditionType = 7249;

        /// <summary>
        /// The identifier for the Accumulated_TimeConditionType ObjectType.
        /// </summary>
        public const uint Accumulated_TimeConditionType = 7310;

        /// <summary>
        /// The identifier for the AmperageConditionType ObjectType.
        /// </summary>
        public const uint AmperageConditionType = 7371;

        /// <summary>
        /// The identifier for the AngleConditionType ObjectType.
        /// </summary>
        public const uint AngleConditionType = 7432;

        /// <summary>
        /// The identifier for the Angular-AccelerationConditionType ObjectType.
        /// </summary>
        public const uint Angular-AccelerationConditionType = 7493;

        /// <summary>
        /// The identifier for the Angular_VelocityConditionType ObjectType.
        /// </summary>
        public const uint Angular_VelocityConditionType = 7554;

        /// <summary>
        /// The identifier for the CommunicationsConditionType ObjectType.
        /// </summary>
        public const uint CommunicationsConditionType = 7615;

        /// <summary>
        /// The identifier for the ConcentrationConditionType ObjectType.
        /// </summary>
        public const uint ConcentrationConditionType = 7676;

        /// <summary>
        /// The identifier for the ConductivityConditionType ObjectType.
        /// </summary>
        public const uint ConductivityConditionType = 7737;

        /// <summary>
        /// The identifier for the Data_RangeConditionType ObjectType.
        /// </summary>
        public const uint Data_RangeConditionType = 7798;

        /// <summary>
        /// The identifier for the DirectionConditionType ObjectType.
        /// </summary>
        public const uint DirectionConditionType = 7859;

        /// <summary>
        /// The identifier for the DisplacementConditionType ObjectType.
        /// </summary>
        public const uint DisplacementConditionType = 7920;

        /// <summary>
        /// The identifier for the Electrical_EnergyConditionType ObjectType.
        /// </summary>
        public const uint Electrical_EnergyConditionType = 7981;

        /// <summary>
        /// The identifier for the Fill_LevelConditionType ObjectType.
        /// </summary>
        public const uint Fill_LevelConditionType = 8042;

        /// <summary>
        /// The identifier for the FlowConditionType ObjectType.
        /// </summary>
        public const uint FlowConditionType = 8103;

        /// <summary>
        /// The identifier for the FrequencyConditionType ObjectType.
        /// </summary>
        public const uint FrequencyConditionType = 8164;

        /// <summary>
        /// The identifier for the HardwareConditionType ObjectType.
        /// </summary>
        public const uint HardwareConditionType = 8225;

        /// <summary>
        /// The identifier for the Linear_ForceConditionType ObjectType.
        /// </summary>
        public const uint Linear_ForceConditionType = 8286;

        /// <summary>
        /// The identifier for the LoadConditionType ObjectType.
        /// </summary>
        public const uint LoadConditionType = 8347;

        /// <summary>
        /// The identifier for the Logic_ProgramConditionType ObjectType.
        /// </summary>
        public const uint Logic_ProgramConditionType = 8408;

        /// <summary>
        /// The identifier for the MassConditionType ObjectType.
        /// </summary>
        public const uint MassConditionType = 8469;

        /// <summary>
        /// The identifier for the Motion_ProgramConditionType ObjectType.
        /// </summary>
        public const uint Motion_ProgramConditionType = 8530;

        /// <summary>
        /// The identifier for the Path_FeedrateConditionType ObjectType.
        /// </summary>
        public const uint Path_FeedrateConditionType = 8591;

        /// <summary>
        /// The identifier for the Path_PositionConditionType ObjectType.
        /// </summary>
        public const uint Path_PositionConditionType = 8652;

        /// <summary>
        /// The identifier for the PHConditionType ObjectType.
        /// </summary>
        public const uint PHConditionType = 8713;

        /// <summary>
        /// The identifier for the PositionConditionType ObjectType.
        /// </summary>
        public const uint PositionConditionType = 8774;

        /// <summary>
        /// The identifier for the Power_FactorConditionType ObjectType.
        /// </summary>
        public const uint Power_FactorConditionType = 8835;

        /// <summary>
        /// The identifier for the PressureConditionType ObjectType.
        /// </summary>
        public const uint PressureConditionType = 8896;

        /// <summary>
        /// The identifier for the ResistanceConditionType ObjectType.
        /// </summary>
        public const uint ResistanceConditionType = 8957;

        /// <summary>
        /// The identifier for the Rotary_VelocityConditionType ObjectType.
        /// </summary>
        public const uint Rotary_VelocityConditionType = 9018;

        /// <summary>
        /// The identifier for the Sound_LevelConditionType ObjectType.
        /// </summary>
        public const uint Sound_LevelConditionType = 9079;

        /// <summary>
        /// The identifier for the StrainConditionType ObjectType.
        /// </summary>
        public const uint StrainConditionType = 9140;

        /// <summary>
        /// The identifier for the SystemConditionType ObjectType.
        /// </summary>
        public const uint SystemConditionType = 9201;

        /// <summary>
        /// The identifier for the TemperatureConditionType ObjectType.
        /// </summary>
        public const uint TemperatureConditionType = 9262;

        /// <summary>
        /// The identifier for the TiltConditionType ObjectType.
        /// </summary>
        public const uint TiltConditionType = 9323;

        /// <summary>
        /// The identifier for the TorqueConditionType ObjectType.
        /// </summary>
        public const uint TorqueConditionType = 9384;

        /// <summary>
        /// The identifier for the VelocityConditionType ObjectType.
        /// </summary>
        public const uint VelocityConditionType = 9445;

        /// <summary>
        /// The identifier for the ViscosityConditionType ObjectType.
        /// </summary>
        public const uint ViscosityConditionType = 9506;

        /// <summary>
        /// The identifier for the VoltageConditionType ObjectType.
        /// </summary>
        public const uint VoltageConditionType = 9567;

        /// <summary>
        /// The identifier for the Volt_AmperageConditionType ObjectType.
        /// </summary>
        public const uint Volt_AmperageConditionType = 9628;

        /// <summary>
        /// The identifier for the VoltAmperageReactiveConditionType ObjectType.
        /// </summary>
        public const uint VoltAmperageReactiveConditionType = 9689;

        /// <summary>
        /// The identifier for the WattageConditionType ObjectType.
        /// </summary>
        public const uint WattageConditionType = 9750;

        /// <summary>
        /// The identifier for the ConfigurationType ObjectType.
        /// </summary>
        public const uint ConfigurationType = 9815;

        /// <summary>
        /// The identifier for the SensorConfigurationType ObjectType.
        /// </summary>
        public const uint SensorConfigurationType = 9828;

        /// <summary>
        /// The identifier for the SensorType ObjectType.
        /// </summary>
        public const uint SensorType = 9842;

        /// <summary>
        /// The identifier for the SourceType ObjectType.
        /// </summary>
        public const uint SourceType = 9855;

        /// <summary>
        /// The identifier for the AssetType ObjectType.
        /// </summary>
        public const uint AssetType = 9869;

        /// <summary>
        /// The identifier for the CuttingToolType ObjectType.
        /// </summary>
        public const uint CuttingToolType = 9888;

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType ObjectType.
        /// </summary>
        public const uint CuttingToolLifeCycleType = 9891;
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
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceUri Variable.
        /// </summary>
        public const uint OpcUaMTConnectNamespaceMetadata_NamespaceUri = 15002;

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceVersion Variable.
        /// </summary>
        public const uint OpcUaMTConnectNamespaceMetadata_NamespaceVersion = 15003;

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespacePublicationDate Variable.
        /// </summary>
        public const uint OpcUaMTConnectNamespaceMetadata_NamespacePublicationDate = 15004;

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_IsNamespaceSubset Variable.
        /// </summary>
        public const uint OpcUaMTConnectNamespaceMetadata_IsNamespaceSubset = 15005;

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_StaticNodeIdTypes Variable.
        /// </summary>
        public const uint OpcUaMTConnectNamespaceMetadata_StaticNodeIdTypes = 15006;

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_StaticNumericNodeIdRange Variable.
        /// </summary>
        public const uint OpcUaMTConnectNamespaceMetadata_StaticNumericNodeIdRange = 15007;

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_StaticStringNodeIdPattern Variable.
        /// </summary>
        public const uint OpcUaMTConnectNamespaceMetadata_StaticStringNodeIdPattern = 15008;

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_Size Variable.
        /// </summary>
        public const uint OpcUaMTConnectNamespaceMetadata_NamespaceFile_Size = 15010;

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_Writable Variable.
        /// </summary>
        public const uint OpcUaMTConnectNamespaceMetadata_NamespaceFile_Writable = 15011;

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_UserWritable Variable.
        /// </summary>
        public const uint OpcUaMTConnectNamespaceMetadata_NamespaceFile_UserWritable = 15012;

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_OpenCount Variable.
        /// </summary>
        public const uint OpcUaMTConnectNamespaceMetadata_NamespaceFile_OpenCount = 15013;

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_Open_InputArguments Variable.
        /// </summary>
        public const uint OpcUaMTConnectNamespaceMetadata_NamespaceFile_Open_InputArguments = 15016;

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_Open_OutputArguments Variable.
        /// </summary>
        public const uint OpcUaMTConnectNamespaceMetadata_NamespaceFile_Open_OutputArguments = 15017;

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_Close_InputArguments Variable.
        /// </summary>
        public const uint OpcUaMTConnectNamespaceMetadata_NamespaceFile_Close_InputArguments = 15019;

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_Read_InputArguments Variable.
        /// </summary>
        public const uint OpcUaMTConnectNamespaceMetadata_NamespaceFile_Read_InputArguments = 15021;

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_Read_OutputArguments Variable.
        /// </summary>
        public const uint OpcUaMTConnectNamespaceMetadata_NamespaceFile_Read_OutputArguments = 15022;

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_Write_InputArguments Variable.
        /// </summary>
        public const uint OpcUaMTConnectNamespaceMetadata_NamespaceFile_Write_InputArguments = 15024;

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_GetPosition_InputArguments Variable.
        /// </summary>
        public const uint OpcUaMTConnectNamespaceMetadata_NamespaceFile_GetPosition_InputArguments = 15026;

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_GetPosition_OutputArguments Variable.
        /// </summary>
        public const uint OpcUaMTConnectNamespaceMetadata_NamespaceFile_GetPosition_OutputArguments = 15027;

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_SetPosition_InputArguments Variable.
        /// </summary>
        public const uint OpcUaMTConnectNamespaceMetadata_NamespaceFile_SetPosition_InputArguments = 15029;

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_DefaultRolePermissions Variable.
        /// </summary>
        public const uint OpcUaMTConnectNamespaceMetadata_DefaultRolePermissions = 15031;

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_DefaultUserRolePermissions Variable.
        /// </summary>
        public const uint OpcUaMTConnectNamespaceMetadata_DefaultUserRolePermissions = 15032;

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_DefaultAccessRestrictions Variable.
        /// </summary>
        public const uint OpcUaMTConnectNamespaceMetadata_DefaultAccessRestrictions = 15033;

        /// <summary>
        /// The identifier for the MTDevicesType_MTConnectVersion Variable.
        /// </summary>
        public const uint MTDevicesType_MTConnectVersion = 6615;

        /// <summary>
        /// The identifier for the MTDevicesType_OPCUAMappingDate Variable.
        /// </summary>
        public const uint MTDevicesType_OPCUAMappingDate = 6616;

        /// <summary>
        /// The identifier for the MTDevicesType_OPCUAVersion Variable.
        /// </summary>
        public const uint MTDevicesType_OPCUAVersion = 6617;

        /// <summary>
        /// The identifier for the MTDeviceType_Availability Variable.
        /// </summary>
        public const uint MTDeviceType_Availability = 6619;

        /// <summary>
        /// The identifier for the MTDeviceType_Manufacturer Variable.
        /// </summary>
        public const uint MTDeviceType_Manufacturer = 6622;

        /// <summary>
        /// The identifier for the MTDeviceType_SerialNumber Variable.
        /// </summary>
        public const uint MTDeviceType_SerialNumber = 6623;

        /// <summary>
        /// The identifier for the MTDeviceType_Configuration Variable.
        /// </summary>
        public const uint MTDeviceType_Configuration = 6624;

        /// <summary>
        /// The identifier for the MTDeviceType_SampleInterval Variable.
        /// </summary>
        public const uint MTDeviceType_SampleInterval = 6625;

        /// <summary>
        /// The identifier for the MTComponentType_Model Variable.
        /// </summary>
        public const uint MTComponentType_Model = 6640;

        /// <summary>
        /// The identifier for the MTComponentType_Station Variable.
        /// </summary>
        public const uint MTComponentType_Station = 6641;

        /// <summary>
        /// The identifier for the MTAxesType_LinearAxesX Variable.
        /// </summary>
        public const uint MTAxesType_LinearAxesX = 6653;

        /// <summary>
        /// The identifier for the MTAxesType_LinearAxesY Variable.
        /// </summary>
        public const uint MTAxesType_LinearAxesY = 6654;

        /// <summary>
        /// The identifier for the MTAxesType_LinearAxesZ Variable.
        /// </summary>
        public const uint MTAxesType_LinearAxesZ = 6655;

        /// <summary>
        /// The identifier for the MTAxesType_RotoryAxesA Variable.
        /// </summary>
        public const uint MTAxesType_RotoryAxesA = 6656;

        /// <summary>
        /// The identifier for the MTAxesType_RotoryAxesB Variable.
        /// </summary>
        public const uint MTAxesType_RotoryAxesB = 6657;

        /// <summary>
        /// The identifier for the MTAxesType_RotoryAxesC Variable.
        /// </summary>
        public const uint MTAxesType_RotoryAxesC = 6658;

        /// <summary>
        /// The identifier for the MTDoorType_DoorState Variable.
        /// </summary>
        public const uint MTDoorType_DoorState = 6681;

        /// <summary>
        /// The identifier for the MTActuatorType_LinearAxesX Variable.
        /// </summary>
        public const uint MTActuatorType_LinearAxesX = 6693;

        /// <summary>
        /// The identifier for the MTSampleDataItemType_CoordinateSystem Variable.
        /// </summary>
        public const uint MTSampleDataItemType_CoordinateSystem = 6700;

        /// <summary>
        /// The identifier for the MTSampleDataItemType_NativeUnits Variable.
        /// </summary>
        public const uint MTSampleDataItemType_NativeUnits = 6701;

        /// <summary>
        /// The identifier for the MTSampleDataItemType_NativeScale Variable.
        /// </summary>
        public const uint MTSampleDataItemType_NativeScale = 6702;

        /// <summary>
        /// The identifier for the MTSampleDataItemType_SampleInterval Variable.
        /// </summary>
        public const uint MTSampleDataItemType_SampleInterval = 6703;

        /// <summary>
        /// The identifier for the MTEventDataItemType_CoordinateSystem Variable.
        /// </summary>
        public const uint MTEventDataItemType_CoordinateSystem = 7067;

        /// <summary>
        /// The identifier for the MTEventDataItemType_SampleInterval Variable.
        /// </summary>
        public const uint MTEventDataItemType_SampleInterval = 7068;

        /// <summary>
        /// The identifier for the MTConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint MTConditionType_EnabledState_Id = 7204;

        /// <summary>
        /// The identifier for the MTConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint MTConditionType_Quality_SourceTimestamp = 7213;

        /// <summary>
        /// The identifier for the MTConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint MTConditionType_LastSeverity_SourceTimestamp = 7215;

        /// <summary>
        /// The identifier for the MTConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint MTConditionType_Comment_SourceTimestamp = 7217;

        /// <summary>
        /// The identifier for the MTConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint MTConditionType_AddComment_InputArguments = 7222;

        /// <summary>
        /// The identifier for the MTConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint MTConditionType_ConditionRefresh_InputArguments = 7224;

        /// <summary>
        /// The identifier for the MTConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint MTConditionType_ConditionRefresh2_InputArguments = 10115;

        /// <summary>
        /// The identifier for the MTConditionType_MTCurrentState Variable.
        /// </summary>
        public const uint MTConditionType_MTCurrentState = 7225;

        /// <summary>
        /// The identifier for the MTConditionType_ActiveState Variable.
        /// </summary>
        public const uint MTConditionType_ActiveState = 7226;

        /// <summary>
        /// The identifier for the MTConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint MTConditionType_ActiveState_Id = 7227;

        /// <summary>
        /// The identifier for the MTConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint MTConditionType_LimitState_CurrentState = 7236;

        /// <summary>
        /// The identifier for the MTConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint MTConditionType_LimitState_CurrentState_Id = 7237;

        /// <summary>
        /// The identifier for the MTConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint MTConditionType_LimitState_LastTransition_Id = 7242;

        /// <summary>
        /// The identifier for the MTConditionType_NativeCode Variable.
        /// </summary>
        public const uint MTConditionType_NativeCode = 7247;

        /// <summary>
        /// The identifier for the MTConditionType_NativeSeverity Variable.
        /// </summary>
        public const uint MTConditionType_NativeSeverity = 7248;

        /// <summary>
        /// The identifier for the AccelerationConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint AccelerationConditionType_EnabledState_Id = 7265;

        /// <summary>
        /// The identifier for the AccelerationConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint AccelerationConditionType_Quality_SourceTimestamp = 7274;

        /// <summary>
        /// The identifier for the AccelerationConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint AccelerationConditionType_LastSeverity_SourceTimestamp = 7276;

        /// <summary>
        /// The identifier for the AccelerationConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint AccelerationConditionType_Comment_SourceTimestamp = 7278;

        /// <summary>
        /// The identifier for the AccelerationConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint AccelerationConditionType_AddComment_InputArguments = 7283;

        /// <summary>
        /// The identifier for the AccelerationConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint AccelerationConditionType_ConditionRefresh_InputArguments = 7285;

        /// <summary>
        /// The identifier for the AccelerationConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint AccelerationConditionType_ConditionRefresh2_InputArguments = 10117;

        /// <summary>
        /// The identifier for the AccelerationConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint AccelerationConditionType_ActiveState_Id = 7288;

        /// <summary>
        /// The identifier for the AccelerationConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint AccelerationConditionType_LimitState_CurrentState = 7297;

        /// <summary>
        /// The identifier for the AccelerationConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint AccelerationConditionType_LimitState_CurrentState_Id = 7298;

        /// <summary>
        /// The identifier for the AccelerationConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint AccelerationConditionType_LimitState_LastTransition_Id = 7303;

        /// <summary>
        /// The identifier for the Accumulated_TimeConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint Accumulated_TimeConditionType_EnabledState_Id = 7326;

        /// <summary>
        /// The identifier for the Accumulated_TimeConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint Accumulated_TimeConditionType_Quality_SourceTimestamp = 7335;

        /// <summary>
        /// The identifier for the Accumulated_TimeConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint Accumulated_TimeConditionType_LastSeverity_SourceTimestamp = 7337;

        /// <summary>
        /// The identifier for the Accumulated_TimeConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint Accumulated_TimeConditionType_Comment_SourceTimestamp = 7339;

        /// <summary>
        /// The identifier for the Accumulated_TimeConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint Accumulated_TimeConditionType_AddComment_InputArguments = 7344;

        /// <summary>
        /// The identifier for the Accumulated_TimeConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint Accumulated_TimeConditionType_ConditionRefresh_InputArguments = 7346;

        /// <summary>
        /// The identifier for the Accumulated_TimeConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint Accumulated_TimeConditionType_ConditionRefresh2_InputArguments = 10119;

        /// <summary>
        /// The identifier for the Accumulated_TimeConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint Accumulated_TimeConditionType_ActiveState_Id = 7349;

        /// <summary>
        /// The identifier for the Accumulated_TimeConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint Accumulated_TimeConditionType_LimitState_CurrentState = 7358;

        /// <summary>
        /// The identifier for the Accumulated_TimeConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint Accumulated_TimeConditionType_LimitState_CurrentState_Id = 7359;

        /// <summary>
        /// The identifier for the Accumulated_TimeConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint Accumulated_TimeConditionType_LimitState_LastTransition_Id = 7364;

        /// <summary>
        /// The identifier for the AmperageConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint AmperageConditionType_EnabledState_Id = 7387;

        /// <summary>
        /// The identifier for the AmperageConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint AmperageConditionType_Quality_SourceTimestamp = 7396;

        /// <summary>
        /// The identifier for the AmperageConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint AmperageConditionType_LastSeverity_SourceTimestamp = 7398;

        /// <summary>
        /// The identifier for the AmperageConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint AmperageConditionType_Comment_SourceTimestamp = 7400;

        /// <summary>
        /// The identifier for the AmperageConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint AmperageConditionType_AddComment_InputArguments = 7405;

        /// <summary>
        /// The identifier for the AmperageConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint AmperageConditionType_ConditionRefresh_InputArguments = 7407;

        /// <summary>
        /// The identifier for the AmperageConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint AmperageConditionType_ConditionRefresh2_InputArguments = 10121;

        /// <summary>
        /// The identifier for the AmperageConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint AmperageConditionType_ActiveState_Id = 7410;

        /// <summary>
        /// The identifier for the AmperageConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint AmperageConditionType_LimitState_CurrentState = 7419;

        /// <summary>
        /// The identifier for the AmperageConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint AmperageConditionType_LimitState_CurrentState_Id = 7420;

        /// <summary>
        /// The identifier for the AmperageConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint AmperageConditionType_LimitState_LastTransition_Id = 7425;

        /// <summary>
        /// The identifier for the AngleConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint AngleConditionType_EnabledState_Id = 7448;

        /// <summary>
        /// The identifier for the AngleConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint AngleConditionType_Quality_SourceTimestamp = 7457;

        /// <summary>
        /// The identifier for the AngleConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint AngleConditionType_LastSeverity_SourceTimestamp = 7459;

        /// <summary>
        /// The identifier for the AngleConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint AngleConditionType_Comment_SourceTimestamp = 7461;

        /// <summary>
        /// The identifier for the AngleConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint AngleConditionType_AddComment_InputArguments = 7466;

        /// <summary>
        /// The identifier for the AngleConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint AngleConditionType_ConditionRefresh_InputArguments = 7468;

        /// <summary>
        /// The identifier for the AngleConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint AngleConditionType_ConditionRefresh2_InputArguments = 10123;

        /// <summary>
        /// The identifier for the AngleConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint AngleConditionType_ActiveState_Id = 7471;

        /// <summary>
        /// The identifier for the AngleConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint AngleConditionType_LimitState_CurrentState = 7480;

        /// <summary>
        /// The identifier for the AngleConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint AngleConditionType_LimitState_CurrentState_Id = 7481;

        /// <summary>
        /// The identifier for the AngleConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint AngleConditionType_LimitState_LastTransition_Id = 7486;

        /// <summary>
        /// The identifier for the Angular-AccelerationConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint Angular-AccelerationConditionType_EnabledState_Id = 7509;

        /// <summary>
        /// The identifier for the Angular-AccelerationConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint Angular-AccelerationConditionType_Quality_SourceTimestamp = 7518;

        /// <summary>
        /// The identifier for the Angular-AccelerationConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint Angular-AccelerationConditionType_LastSeverity_SourceTimestamp = 7520;

        /// <summary>
        /// The identifier for the Angular-AccelerationConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint Angular-AccelerationConditionType_Comment_SourceTimestamp = 7522;

        /// <summary>
        /// The identifier for the Angular-AccelerationConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint Angular-AccelerationConditionType_AddComment_InputArguments = 7527;

        /// <summary>
        /// The identifier for the Angular-AccelerationConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint Angular-AccelerationConditionType_ConditionRefresh_InputArguments = 7529;

        /// <summary>
        /// The identifier for the Angular-AccelerationConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint Angular-AccelerationConditionType_ConditionRefresh2_InputArguments = 10125;

        /// <summary>
        /// The identifier for the Angular-AccelerationConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint Angular-AccelerationConditionType_ActiveState_Id = 7532;

        /// <summary>
        /// The identifier for the Angular-AccelerationConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint Angular-AccelerationConditionType_LimitState_CurrentState = 7541;

        /// <summary>
        /// The identifier for the Angular-AccelerationConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint Angular-AccelerationConditionType_LimitState_CurrentState_Id = 7542;

        /// <summary>
        /// The identifier for the Angular-AccelerationConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint Angular-AccelerationConditionType_LimitState_LastTransition_Id = 7547;

        /// <summary>
        /// The identifier for the Angular_VelocityConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint Angular_VelocityConditionType_EnabledState_Id = 7570;

        /// <summary>
        /// The identifier for the Angular_VelocityConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint Angular_VelocityConditionType_Quality_SourceTimestamp = 7579;

        /// <summary>
        /// The identifier for the Angular_VelocityConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint Angular_VelocityConditionType_LastSeverity_SourceTimestamp = 7581;

        /// <summary>
        /// The identifier for the Angular_VelocityConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint Angular_VelocityConditionType_Comment_SourceTimestamp = 7583;

        /// <summary>
        /// The identifier for the Angular_VelocityConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint Angular_VelocityConditionType_AddComment_InputArguments = 7588;

        /// <summary>
        /// The identifier for the Angular_VelocityConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint Angular_VelocityConditionType_ConditionRefresh_InputArguments = 7590;

        /// <summary>
        /// The identifier for the Angular_VelocityConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint Angular_VelocityConditionType_ConditionRefresh2_InputArguments = 10127;

        /// <summary>
        /// The identifier for the Angular_VelocityConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint Angular_VelocityConditionType_ActiveState_Id = 7593;

        /// <summary>
        /// The identifier for the Angular_VelocityConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint Angular_VelocityConditionType_LimitState_CurrentState = 7602;

        /// <summary>
        /// The identifier for the Angular_VelocityConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint Angular_VelocityConditionType_LimitState_CurrentState_Id = 7603;

        /// <summary>
        /// The identifier for the Angular_VelocityConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint Angular_VelocityConditionType_LimitState_LastTransition_Id = 7608;

        /// <summary>
        /// The identifier for the CommunicationsConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint CommunicationsConditionType_EnabledState_Id = 7631;

        /// <summary>
        /// The identifier for the CommunicationsConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint CommunicationsConditionType_Quality_SourceTimestamp = 7640;

        /// <summary>
        /// The identifier for the CommunicationsConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint CommunicationsConditionType_LastSeverity_SourceTimestamp = 7642;

        /// <summary>
        /// The identifier for the CommunicationsConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint CommunicationsConditionType_Comment_SourceTimestamp = 7644;

        /// <summary>
        /// The identifier for the CommunicationsConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint CommunicationsConditionType_AddComment_InputArguments = 7649;

        /// <summary>
        /// The identifier for the CommunicationsConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint CommunicationsConditionType_ConditionRefresh_InputArguments = 7651;

        /// <summary>
        /// The identifier for the CommunicationsConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint CommunicationsConditionType_ConditionRefresh2_InputArguments = 10129;

        /// <summary>
        /// The identifier for the CommunicationsConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint CommunicationsConditionType_ActiveState_Id = 7654;

        /// <summary>
        /// The identifier for the CommunicationsConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint CommunicationsConditionType_LimitState_CurrentState = 7663;

        /// <summary>
        /// The identifier for the CommunicationsConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint CommunicationsConditionType_LimitState_CurrentState_Id = 7664;

        /// <summary>
        /// The identifier for the CommunicationsConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint CommunicationsConditionType_LimitState_LastTransition_Id = 7669;

        /// <summary>
        /// The identifier for the ConcentrationConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint ConcentrationConditionType_EnabledState_Id = 7692;

        /// <summary>
        /// The identifier for the ConcentrationConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint ConcentrationConditionType_Quality_SourceTimestamp = 7701;

        /// <summary>
        /// The identifier for the ConcentrationConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint ConcentrationConditionType_LastSeverity_SourceTimestamp = 7703;

        /// <summary>
        /// The identifier for the ConcentrationConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint ConcentrationConditionType_Comment_SourceTimestamp = 7705;

        /// <summary>
        /// The identifier for the ConcentrationConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint ConcentrationConditionType_AddComment_InputArguments = 7710;

        /// <summary>
        /// The identifier for the ConcentrationConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint ConcentrationConditionType_ConditionRefresh_InputArguments = 7712;

        /// <summary>
        /// The identifier for the ConcentrationConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint ConcentrationConditionType_ConditionRefresh2_InputArguments = 10131;

        /// <summary>
        /// The identifier for the ConcentrationConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint ConcentrationConditionType_ActiveState_Id = 7715;

        /// <summary>
        /// The identifier for the ConcentrationConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint ConcentrationConditionType_LimitState_CurrentState = 7724;

        /// <summary>
        /// The identifier for the ConcentrationConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint ConcentrationConditionType_LimitState_CurrentState_Id = 7725;

        /// <summary>
        /// The identifier for the ConcentrationConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint ConcentrationConditionType_LimitState_LastTransition_Id = 7730;

        /// <summary>
        /// The identifier for the ConductivityConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint ConductivityConditionType_EnabledState_Id = 7753;

        /// <summary>
        /// The identifier for the ConductivityConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint ConductivityConditionType_Quality_SourceTimestamp = 7762;

        /// <summary>
        /// The identifier for the ConductivityConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint ConductivityConditionType_LastSeverity_SourceTimestamp = 7764;

        /// <summary>
        /// The identifier for the ConductivityConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint ConductivityConditionType_Comment_SourceTimestamp = 7766;

        /// <summary>
        /// The identifier for the ConductivityConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint ConductivityConditionType_AddComment_InputArguments = 7771;

        /// <summary>
        /// The identifier for the ConductivityConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint ConductivityConditionType_ConditionRefresh_InputArguments = 7773;

        /// <summary>
        /// The identifier for the ConductivityConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint ConductivityConditionType_ConditionRefresh2_InputArguments = 10133;

        /// <summary>
        /// The identifier for the ConductivityConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint ConductivityConditionType_ActiveState_Id = 7776;

        /// <summary>
        /// The identifier for the ConductivityConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint ConductivityConditionType_LimitState_CurrentState = 7785;

        /// <summary>
        /// The identifier for the ConductivityConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint ConductivityConditionType_LimitState_CurrentState_Id = 7786;

        /// <summary>
        /// The identifier for the ConductivityConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint ConductivityConditionType_LimitState_LastTransition_Id = 7791;

        /// <summary>
        /// The identifier for the Data_RangeConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint Data_RangeConditionType_EnabledState_Id = 7814;

        /// <summary>
        /// The identifier for the Data_RangeConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint Data_RangeConditionType_Quality_SourceTimestamp = 7823;

        /// <summary>
        /// The identifier for the Data_RangeConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint Data_RangeConditionType_LastSeverity_SourceTimestamp = 7825;

        /// <summary>
        /// The identifier for the Data_RangeConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint Data_RangeConditionType_Comment_SourceTimestamp = 7827;

        /// <summary>
        /// The identifier for the Data_RangeConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint Data_RangeConditionType_AddComment_InputArguments = 7832;

        /// <summary>
        /// The identifier for the Data_RangeConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint Data_RangeConditionType_ConditionRefresh_InputArguments = 7834;

        /// <summary>
        /// The identifier for the Data_RangeConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint Data_RangeConditionType_ConditionRefresh2_InputArguments = 10135;

        /// <summary>
        /// The identifier for the Data_RangeConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint Data_RangeConditionType_ActiveState_Id = 7837;

        /// <summary>
        /// The identifier for the Data_RangeConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint Data_RangeConditionType_LimitState_CurrentState = 7846;

        /// <summary>
        /// The identifier for the Data_RangeConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint Data_RangeConditionType_LimitState_CurrentState_Id = 7847;

        /// <summary>
        /// The identifier for the Data_RangeConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint Data_RangeConditionType_LimitState_LastTransition_Id = 7852;

        /// <summary>
        /// The identifier for the DirectionConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint DirectionConditionType_EnabledState_Id = 7875;

        /// <summary>
        /// The identifier for the DirectionConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint DirectionConditionType_Quality_SourceTimestamp = 7884;

        /// <summary>
        /// The identifier for the DirectionConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint DirectionConditionType_LastSeverity_SourceTimestamp = 7886;

        /// <summary>
        /// The identifier for the DirectionConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint DirectionConditionType_Comment_SourceTimestamp = 7888;

        /// <summary>
        /// The identifier for the DirectionConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint DirectionConditionType_AddComment_InputArguments = 7893;

        /// <summary>
        /// The identifier for the DirectionConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint DirectionConditionType_ConditionRefresh_InputArguments = 7895;

        /// <summary>
        /// The identifier for the DirectionConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint DirectionConditionType_ConditionRefresh2_InputArguments = 10137;

        /// <summary>
        /// The identifier for the DirectionConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint DirectionConditionType_ActiveState_Id = 7898;

        /// <summary>
        /// The identifier for the DirectionConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint DirectionConditionType_LimitState_CurrentState = 7907;

        /// <summary>
        /// The identifier for the DirectionConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint DirectionConditionType_LimitState_CurrentState_Id = 7908;

        /// <summary>
        /// The identifier for the DirectionConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint DirectionConditionType_LimitState_LastTransition_Id = 7913;

        /// <summary>
        /// The identifier for the DisplacementConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint DisplacementConditionType_EnabledState_Id = 7936;

        /// <summary>
        /// The identifier for the DisplacementConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint DisplacementConditionType_Quality_SourceTimestamp = 7945;

        /// <summary>
        /// The identifier for the DisplacementConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint DisplacementConditionType_LastSeverity_SourceTimestamp = 7947;

        /// <summary>
        /// The identifier for the DisplacementConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint DisplacementConditionType_Comment_SourceTimestamp = 7949;

        /// <summary>
        /// The identifier for the DisplacementConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint DisplacementConditionType_AddComment_InputArguments = 7954;

        /// <summary>
        /// The identifier for the DisplacementConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint DisplacementConditionType_ConditionRefresh_InputArguments = 7956;

        /// <summary>
        /// The identifier for the DisplacementConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint DisplacementConditionType_ConditionRefresh2_InputArguments = 10139;

        /// <summary>
        /// The identifier for the DisplacementConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint DisplacementConditionType_ActiveState_Id = 7959;

        /// <summary>
        /// The identifier for the DisplacementConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint DisplacementConditionType_LimitState_CurrentState = 7968;

        /// <summary>
        /// The identifier for the DisplacementConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint DisplacementConditionType_LimitState_CurrentState_Id = 7969;

        /// <summary>
        /// The identifier for the DisplacementConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint DisplacementConditionType_LimitState_LastTransition_Id = 7974;

        /// <summary>
        /// The identifier for the Electrical_EnergyConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint Electrical_EnergyConditionType_EnabledState_Id = 7997;

        /// <summary>
        /// The identifier for the Electrical_EnergyConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint Electrical_EnergyConditionType_Quality_SourceTimestamp = 8006;

        /// <summary>
        /// The identifier for the Electrical_EnergyConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint Electrical_EnergyConditionType_LastSeverity_SourceTimestamp = 8008;

        /// <summary>
        /// The identifier for the Electrical_EnergyConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint Electrical_EnergyConditionType_Comment_SourceTimestamp = 8010;

        /// <summary>
        /// The identifier for the Electrical_EnergyConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint Electrical_EnergyConditionType_AddComment_InputArguments = 8015;

        /// <summary>
        /// The identifier for the Electrical_EnergyConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint Electrical_EnergyConditionType_ConditionRefresh_InputArguments = 8017;

        /// <summary>
        /// The identifier for the Electrical_EnergyConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint Electrical_EnergyConditionType_ConditionRefresh2_InputArguments = 10141;

        /// <summary>
        /// The identifier for the Electrical_EnergyConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint Electrical_EnergyConditionType_ActiveState_Id = 8020;

        /// <summary>
        /// The identifier for the Electrical_EnergyConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint Electrical_EnergyConditionType_LimitState_CurrentState = 8029;

        /// <summary>
        /// The identifier for the Electrical_EnergyConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint Electrical_EnergyConditionType_LimitState_CurrentState_Id = 8030;

        /// <summary>
        /// The identifier for the Electrical_EnergyConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint Electrical_EnergyConditionType_LimitState_LastTransition_Id = 8035;

        /// <summary>
        /// The identifier for the Fill_LevelConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint Fill_LevelConditionType_EnabledState_Id = 8058;

        /// <summary>
        /// The identifier for the Fill_LevelConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint Fill_LevelConditionType_Quality_SourceTimestamp = 8067;

        /// <summary>
        /// The identifier for the Fill_LevelConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint Fill_LevelConditionType_LastSeverity_SourceTimestamp = 8069;

        /// <summary>
        /// The identifier for the Fill_LevelConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint Fill_LevelConditionType_Comment_SourceTimestamp = 8071;

        /// <summary>
        /// The identifier for the Fill_LevelConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint Fill_LevelConditionType_AddComment_InputArguments = 8076;

        /// <summary>
        /// The identifier for the Fill_LevelConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint Fill_LevelConditionType_ConditionRefresh_InputArguments = 8078;

        /// <summary>
        /// The identifier for the Fill_LevelConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint Fill_LevelConditionType_ConditionRefresh2_InputArguments = 10143;

        /// <summary>
        /// The identifier for the Fill_LevelConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint Fill_LevelConditionType_ActiveState_Id = 8081;

        /// <summary>
        /// The identifier for the Fill_LevelConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint Fill_LevelConditionType_LimitState_CurrentState = 8090;

        /// <summary>
        /// The identifier for the Fill_LevelConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint Fill_LevelConditionType_LimitState_CurrentState_Id = 8091;

        /// <summary>
        /// The identifier for the Fill_LevelConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint Fill_LevelConditionType_LimitState_LastTransition_Id = 8096;

        /// <summary>
        /// The identifier for the FlowConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint FlowConditionType_EnabledState_Id = 8119;

        /// <summary>
        /// The identifier for the FlowConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint FlowConditionType_Quality_SourceTimestamp = 8128;

        /// <summary>
        /// The identifier for the FlowConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint FlowConditionType_LastSeverity_SourceTimestamp = 8130;

        /// <summary>
        /// The identifier for the FlowConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint FlowConditionType_Comment_SourceTimestamp = 8132;

        /// <summary>
        /// The identifier for the FlowConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint FlowConditionType_AddComment_InputArguments = 8137;

        /// <summary>
        /// The identifier for the FlowConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint FlowConditionType_ConditionRefresh_InputArguments = 8139;

        /// <summary>
        /// The identifier for the FlowConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint FlowConditionType_ConditionRefresh2_InputArguments = 10145;

        /// <summary>
        /// The identifier for the FlowConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint FlowConditionType_ActiveState_Id = 8142;

        /// <summary>
        /// The identifier for the FlowConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint FlowConditionType_LimitState_CurrentState = 8151;

        /// <summary>
        /// The identifier for the FlowConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint FlowConditionType_LimitState_CurrentState_Id = 8152;

        /// <summary>
        /// The identifier for the FlowConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint FlowConditionType_LimitState_LastTransition_Id = 8157;

        /// <summary>
        /// The identifier for the FrequencyConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint FrequencyConditionType_EnabledState_Id = 8180;

        /// <summary>
        /// The identifier for the FrequencyConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint FrequencyConditionType_Quality_SourceTimestamp = 8189;

        /// <summary>
        /// The identifier for the FrequencyConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint FrequencyConditionType_LastSeverity_SourceTimestamp = 8191;

        /// <summary>
        /// The identifier for the FrequencyConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint FrequencyConditionType_Comment_SourceTimestamp = 8193;

        /// <summary>
        /// The identifier for the FrequencyConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint FrequencyConditionType_AddComment_InputArguments = 8198;

        /// <summary>
        /// The identifier for the FrequencyConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint FrequencyConditionType_ConditionRefresh_InputArguments = 8200;

        /// <summary>
        /// The identifier for the FrequencyConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint FrequencyConditionType_ConditionRefresh2_InputArguments = 10147;

        /// <summary>
        /// The identifier for the FrequencyConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint FrequencyConditionType_ActiveState_Id = 8203;

        /// <summary>
        /// The identifier for the FrequencyConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint FrequencyConditionType_LimitState_CurrentState = 8212;

        /// <summary>
        /// The identifier for the FrequencyConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint FrequencyConditionType_LimitState_CurrentState_Id = 8213;

        /// <summary>
        /// The identifier for the FrequencyConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint FrequencyConditionType_LimitState_LastTransition_Id = 8218;

        /// <summary>
        /// The identifier for the HardwareConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint HardwareConditionType_EnabledState_Id = 8241;

        /// <summary>
        /// The identifier for the HardwareConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint HardwareConditionType_Quality_SourceTimestamp = 8250;

        /// <summary>
        /// The identifier for the HardwareConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint HardwareConditionType_LastSeverity_SourceTimestamp = 8252;

        /// <summary>
        /// The identifier for the HardwareConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint HardwareConditionType_Comment_SourceTimestamp = 8254;

        /// <summary>
        /// The identifier for the HardwareConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint HardwareConditionType_AddComment_InputArguments = 8259;

        /// <summary>
        /// The identifier for the HardwareConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint HardwareConditionType_ConditionRefresh_InputArguments = 8261;

        /// <summary>
        /// The identifier for the HardwareConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint HardwareConditionType_ConditionRefresh2_InputArguments = 10149;

        /// <summary>
        /// The identifier for the HardwareConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint HardwareConditionType_ActiveState_Id = 8264;

        /// <summary>
        /// The identifier for the HardwareConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint HardwareConditionType_LimitState_CurrentState = 8273;

        /// <summary>
        /// The identifier for the HardwareConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint HardwareConditionType_LimitState_CurrentState_Id = 8274;

        /// <summary>
        /// The identifier for the HardwareConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint HardwareConditionType_LimitState_LastTransition_Id = 8279;

        /// <summary>
        /// The identifier for the Linear_ForceConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint Linear_ForceConditionType_EnabledState_Id = 8302;

        /// <summary>
        /// The identifier for the Linear_ForceConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint Linear_ForceConditionType_Quality_SourceTimestamp = 8311;

        /// <summary>
        /// The identifier for the Linear_ForceConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint Linear_ForceConditionType_LastSeverity_SourceTimestamp = 8313;

        /// <summary>
        /// The identifier for the Linear_ForceConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint Linear_ForceConditionType_Comment_SourceTimestamp = 8315;

        /// <summary>
        /// The identifier for the Linear_ForceConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint Linear_ForceConditionType_AddComment_InputArguments = 8320;

        /// <summary>
        /// The identifier for the Linear_ForceConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint Linear_ForceConditionType_ConditionRefresh_InputArguments = 8322;

        /// <summary>
        /// The identifier for the Linear_ForceConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint Linear_ForceConditionType_ConditionRefresh2_InputArguments = 10151;

        /// <summary>
        /// The identifier for the Linear_ForceConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint Linear_ForceConditionType_ActiveState_Id = 8325;

        /// <summary>
        /// The identifier for the Linear_ForceConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint Linear_ForceConditionType_LimitState_CurrentState = 8334;

        /// <summary>
        /// The identifier for the Linear_ForceConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint Linear_ForceConditionType_LimitState_CurrentState_Id = 8335;

        /// <summary>
        /// The identifier for the Linear_ForceConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint Linear_ForceConditionType_LimitState_LastTransition_Id = 8340;

        /// <summary>
        /// The identifier for the LoadConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint LoadConditionType_EnabledState_Id = 8363;

        /// <summary>
        /// The identifier for the LoadConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint LoadConditionType_Quality_SourceTimestamp = 8372;

        /// <summary>
        /// The identifier for the LoadConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint LoadConditionType_LastSeverity_SourceTimestamp = 8374;

        /// <summary>
        /// The identifier for the LoadConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint LoadConditionType_Comment_SourceTimestamp = 8376;

        /// <summary>
        /// The identifier for the LoadConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint LoadConditionType_AddComment_InputArguments = 8381;

        /// <summary>
        /// The identifier for the LoadConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint LoadConditionType_ConditionRefresh_InputArguments = 8383;

        /// <summary>
        /// The identifier for the LoadConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint LoadConditionType_ConditionRefresh2_InputArguments = 10153;

        /// <summary>
        /// The identifier for the LoadConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint LoadConditionType_ActiveState_Id = 8386;

        /// <summary>
        /// The identifier for the LoadConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint LoadConditionType_LimitState_CurrentState = 8395;

        /// <summary>
        /// The identifier for the LoadConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint LoadConditionType_LimitState_CurrentState_Id = 8396;

        /// <summary>
        /// The identifier for the LoadConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint LoadConditionType_LimitState_LastTransition_Id = 8401;

        /// <summary>
        /// The identifier for the Logic_ProgramConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint Logic_ProgramConditionType_EnabledState_Id = 8424;

        /// <summary>
        /// The identifier for the Logic_ProgramConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint Logic_ProgramConditionType_Quality_SourceTimestamp = 8433;

        /// <summary>
        /// The identifier for the Logic_ProgramConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint Logic_ProgramConditionType_LastSeverity_SourceTimestamp = 8435;

        /// <summary>
        /// The identifier for the Logic_ProgramConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint Logic_ProgramConditionType_Comment_SourceTimestamp = 8437;

        /// <summary>
        /// The identifier for the Logic_ProgramConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint Logic_ProgramConditionType_AddComment_InputArguments = 8442;

        /// <summary>
        /// The identifier for the Logic_ProgramConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint Logic_ProgramConditionType_ConditionRefresh_InputArguments = 8444;

        /// <summary>
        /// The identifier for the Logic_ProgramConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint Logic_ProgramConditionType_ConditionRefresh2_InputArguments = 10155;

        /// <summary>
        /// The identifier for the Logic_ProgramConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint Logic_ProgramConditionType_ActiveState_Id = 8447;

        /// <summary>
        /// The identifier for the Logic_ProgramConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint Logic_ProgramConditionType_LimitState_CurrentState = 8456;

        /// <summary>
        /// The identifier for the Logic_ProgramConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint Logic_ProgramConditionType_LimitState_CurrentState_Id = 8457;

        /// <summary>
        /// The identifier for the Logic_ProgramConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint Logic_ProgramConditionType_LimitState_LastTransition_Id = 8462;

        /// <summary>
        /// The identifier for the MassConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint MassConditionType_EnabledState_Id = 8485;

        /// <summary>
        /// The identifier for the MassConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint MassConditionType_Quality_SourceTimestamp = 8494;

        /// <summary>
        /// The identifier for the MassConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint MassConditionType_LastSeverity_SourceTimestamp = 8496;

        /// <summary>
        /// The identifier for the MassConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint MassConditionType_Comment_SourceTimestamp = 8498;

        /// <summary>
        /// The identifier for the MassConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint MassConditionType_AddComment_InputArguments = 8503;

        /// <summary>
        /// The identifier for the MassConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint MassConditionType_ConditionRefresh_InputArguments = 8505;

        /// <summary>
        /// The identifier for the MassConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint MassConditionType_ConditionRefresh2_InputArguments = 10157;

        /// <summary>
        /// The identifier for the MassConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint MassConditionType_ActiveState_Id = 8508;

        /// <summary>
        /// The identifier for the MassConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint MassConditionType_LimitState_CurrentState = 8517;

        /// <summary>
        /// The identifier for the MassConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint MassConditionType_LimitState_CurrentState_Id = 8518;

        /// <summary>
        /// The identifier for the MassConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint MassConditionType_LimitState_LastTransition_Id = 8523;

        /// <summary>
        /// The identifier for the Motion_ProgramConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint Motion_ProgramConditionType_EnabledState_Id = 8546;

        /// <summary>
        /// The identifier for the Motion_ProgramConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint Motion_ProgramConditionType_Quality_SourceTimestamp = 8555;

        /// <summary>
        /// The identifier for the Motion_ProgramConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint Motion_ProgramConditionType_LastSeverity_SourceTimestamp = 8557;

        /// <summary>
        /// The identifier for the Motion_ProgramConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint Motion_ProgramConditionType_Comment_SourceTimestamp = 8559;

        /// <summary>
        /// The identifier for the Motion_ProgramConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint Motion_ProgramConditionType_AddComment_InputArguments = 8564;

        /// <summary>
        /// The identifier for the Motion_ProgramConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint Motion_ProgramConditionType_ConditionRefresh_InputArguments = 8566;

        /// <summary>
        /// The identifier for the Motion_ProgramConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint Motion_ProgramConditionType_ConditionRefresh2_InputArguments = 10159;

        /// <summary>
        /// The identifier for the Motion_ProgramConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint Motion_ProgramConditionType_ActiveState_Id = 8569;

        /// <summary>
        /// The identifier for the Motion_ProgramConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint Motion_ProgramConditionType_LimitState_CurrentState = 8578;

        /// <summary>
        /// The identifier for the Motion_ProgramConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint Motion_ProgramConditionType_LimitState_CurrentState_Id = 8579;

        /// <summary>
        /// The identifier for the Motion_ProgramConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint Motion_ProgramConditionType_LimitState_LastTransition_Id = 8584;

        /// <summary>
        /// The identifier for the Path_FeedrateConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint Path_FeedrateConditionType_EnabledState_Id = 8607;

        /// <summary>
        /// The identifier for the Path_FeedrateConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint Path_FeedrateConditionType_Quality_SourceTimestamp = 8616;

        /// <summary>
        /// The identifier for the Path_FeedrateConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint Path_FeedrateConditionType_LastSeverity_SourceTimestamp = 8618;

        /// <summary>
        /// The identifier for the Path_FeedrateConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint Path_FeedrateConditionType_Comment_SourceTimestamp = 8620;

        /// <summary>
        /// The identifier for the Path_FeedrateConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint Path_FeedrateConditionType_AddComment_InputArguments = 8625;

        /// <summary>
        /// The identifier for the Path_FeedrateConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint Path_FeedrateConditionType_ConditionRefresh_InputArguments = 8627;

        /// <summary>
        /// The identifier for the Path_FeedrateConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint Path_FeedrateConditionType_ConditionRefresh2_InputArguments = 10161;

        /// <summary>
        /// The identifier for the Path_FeedrateConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint Path_FeedrateConditionType_ActiveState_Id = 8630;

        /// <summary>
        /// The identifier for the Path_FeedrateConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint Path_FeedrateConditionType_LimitState_CurrentState = 8639;

        /// <summary>
        /// The identifier for the Path_FeedrateConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint Path_FeedrateConditionType_LimitState_CurrentState_Id = 8640;

        /// <summary>
        /// The identifier for the Path_FeedrateConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint Path_FeedrateConditionType_LimitState_LastTransition_Id = 8645;

        /// <summary>
        /// The identifier for the Path_PositionConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint Path_PositionConditionType_EnabledState_Id = 8668;

        /// <summary>
        /// The identifier for the Path_PositionConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint Path_PositionConditionType_Quality_SourceTimestamp = 8677;

        /// <summary>
        /// The identifier for the Path_PositionConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint Path_PositionConditionType_LastSeverity_SourceTimestamp = 8679;

        /// <summary>
        /// The identifier for the Path_PositionConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint Path_PositionConditionType_Comment_SourceTimestamp = 8681;

        /// <summary>
        /// The identifier for the Path_PositionConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint Path_PositionConditionType_AddComment_InputArguments = 8686;

        /// <summary>
        /// The identifier for the Path_PositionConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint Path_PositionConditionType_ConditionRefresh_InputArguments = 8688;

        /// <summary>
        /// The identifier for the Path_PositionConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint Path_PositionConditionType_ConditionRefresh2_InputArguments = 10163;

        /// <summary>
        /// The identifier for the Path_PositionConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint Path_PositionConditionType_ActiveState_Id = 8691;

        /// <summary>
        /// The identifier for the Path_PositionConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint Path_PositionConditionType_LimitState_CurrentState = 8700;

        /// <summary>
        /// The identifier for the Path_PositionConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint Path_PositionConditionType_LimitState_CurrentState_Id = 8701;

        /// <summary>
        /// The identifier for the Path_PositionConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint Path_PositionConditionType_LimitState_LastTransition_Id = 8706;

        /// <summary>
        /// The identifier for the PHConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint PHConditionType_EnabledState_Id = 8729;

        /// <summary>
        /// The identifier for the PHConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint PHConditionType_Quality_SourceTimestamp = 8738;

        /// <summary>
        /// The identifier for the PHConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint PHConditionType_LastSeverity_SourceTimestamp = 8740;

        /// <summary>
        /// The identifier for the PHConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint PHConditionType_Comment_SourceTimestamp = 8742;

        /// <summary>
        /// The identifier for the PHConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint PHConditionType_AddComment_InputArguments = 8747;

        /// <summary>
        /// The identifier for the PHConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint PHConditionType_ConditionRefresh_InputArguments = 8749;

        /// <summary>
        /// The identifier for the PHConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint PHConditionType_ConditionRefresh2_InputArguments = 10165;

        /// <summary>
        /// The identifier for the PHConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint PHConditionType_ActiveState_Id = 8752;

        /// <summary>
        /// The identifier for the PHConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint PHConditionType_LimitState_CurrentState = 8761;

        /// <summary>
        /// The identifier for the PHConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint PHConditionType_LimitState_CurrentState_Id = 8762;

        /// <summary>
        /// The identifier for the PHConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint PHConditionType_LimitState_LastTransition_Id = 8767;

        /// <summary>
        /// The identifier for the PositionConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint PositionConditionType_EnabledState_Id = 8790;

        /// <summary>
        /// The identifier for the PositionConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint PositionConditionType_Quality_SourceTimestamp = 8799;

        /// <summary>
        /// The identifier for the PositionConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint PositionConditionType_LastSeverity_SourceTimestamp = 8801;

        /// <summary>
        /// The identifier for the PositionConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint PositionConditionType_Comment_SourceTimestamp = 8803;

        /// <summary>
        /// The identifier for the PositionConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint PositionConditionType_AddComment_InputArguments = 8808;

        /// <summary>
        /// The identifier for the PositionConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint PositionConditionType_ConditionRefresh_InputArguments = 8810;

        /// <summary>
        /// The identifier for the PositionConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint PositionConditionType_ConditionRefresh2_InputArguments = 10167;

        /// <summary>
        /// The identifier for the PositionConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint PositionConditionType_ActiveState_Id = 8813;

        /// <summary>
        /// The identifier for the PositionConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint PositionConditionType_LimitState_CurrentState = 8822;

        /// <summary>
        /// The identifier for the PositionConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint PositionConditionType_LimitState_CurrentState_Id = 8823;

        /// <summary>
        /// The identifier for the PositionConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint PositionConditionType_LimitState_LastTransition_Id = 8828;

        /// <summary>
        /// The identifier for the Power_FactorConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint Power_FactorConditionType_EnabledState_Id = 8851;

        /// <summary>
        /// The identifier for the Power_FactorConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint Power_FactorConditionType_Quality_SourceTimestamp = 8860;

        /// <summary>
        /// The identifier for the Power_FactorConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint Power_FactorConditionType_LastSeverity_SourceTimestamp = 8862;

        /// <summary>
        /// The identifier for the Power_FactorConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint Power_FactorConditionType_Comment_SourceTimestamp = 8864;

        /// <summary>
        /// The identifier for the Power_FactorConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint Power_FactorConditionType_AddComment_InputArguments = 8869;

        /// <summary>
        /// The identifier for the Power_FactorConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint Power_FactorConditionType_ConditionRefresh_InputArguments = 8871;

        /// <summary>
        /// The identifier for the Power_FactorConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint Power_FactorConditionType_ConditionRefresh2_InputArguments = 10169;

        /// <summary>
        /// The identifier for the Power_FactorConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint Power_FactorConditionType_ActiveState_Id = 8874;

        /// <summary>
        /// The identifier for the Power_FactorConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint Power_FactorConditionType_LimitState_CurrentState = 8883;

        /// <summary>
        /// The identifier for the Power_FactorConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint Power_FactorConditionType_LimitState_CurrentState_Id = 8884;

        /// <summary>
        /// The identifier for the Power_FactorConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint Power_FactorConditionType_LimitState_LastTransition_Id = 8889;

        /// <summary>
        /// The identifier for the PressureConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint PressureConditionType_EnabledState_Id = 8912;

        /// <summary>
        /// The identifier for the PressureConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint PressureConditionType_Quality_SourceTimestamp = 8921;

        /// <summary>
        /// The identifier for the PressureConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint PressureConditionType_LastSeverity_SourceTimestamp = 8923;

        /// <summary>
        /// The identifier for the PressureConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint PressureConditionType_Comment_SourceTimestamp = 8925;

        /// <summary>
        /// The identifier for the PressureConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint PressureConditionType_AddComment_InputArguments = 8930;

        /// <summary>
        /// The identifier for the PressureConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint PressureConditionType_ConditionRefresh_InputArguments = 8932;

        /// <summary>
        /// The identifier for the PressureConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint PressureConditionType_ConditionRefresh2_InputArguments = 10171;

        /// <summary>
        /// The identifier for the PressureConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint PressureConditionType_ActiveState_Id = 8935;

        /// <summary>
        /// The identifier for the PressureConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint PressureConditionType_LimitState_CurrentState = 8944;

        /// <summary>
        /// The identifier for the PressureConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint PressureConditionType_LimitState_CurrentState_Id = 8945;

        /// <summary>
        /// The identifier for the PressureConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint PressureConditionType_LimitState_LastTransition_Id = 8950;

        /// <summary>
        /// The identifier for the ResistanceConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint ResistanceConditionType_EnabledState_Id = 8973;

        /// <summary>
        /// The identifier for the ResistanceConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint ResistanceConditionType_Quality_SourceTimestamp = 8982;

        /// <summary>
        /// The identifier for the ResistanceConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint ResistanceConditionType_LastSeverity_SourceTimestamp = 8984;

        /// <summary>
        /// The identifier for the ResistanceConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint ResistanceConditionType_Comment_SourceTimestamp = 8986;

        /// <summary>
        /// The identifier for the ResistanceConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint ResistanceConditionType_AddComment_InputArguments = 8991;

        /// <summary>
        /// The identifier for the ResistanceConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint ResistanceConditionType_ConditionRefresh_InputArguments = 8993;

        /// <summary>
        /// The identifier for the ResistanceConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint ResistanceConditionType_ConditionRefresh2_InputArguments = 10173;

        /// <summary>
        /// The identifier for the ResistanceConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint ResistanceConditionType_ActiveState_Id = 8996;

        /// <summary>
        /// The identifier for the ResistanceConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint ResistanceConditionType_LimitState_CurrentState = 9005;

        /// <summary>
        /// The identifier for the ResistanceConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint ResistanceConditionType_LimitState_CurrentState_Id = 9006;

        /// <summary>
        /// The identifier for the ResistanceConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint ResistanceConditionType_LimitState_LastTransition_Id = 9011;

        /// <summary>
        /// The identifier for the Rotary_VelocityConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint Rotary_VelocityConditionType_EnabledState_Id = 9034;

        /// <summary>
        /// The identifier for the Rotary_VelocityConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint Rotary_VelocityConditionType_Quality_SourceTimestamp = 9043;

        /// <summary>
        /// The identifier for the Rotary_VelocityConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint Rotary_VelocityConditionType_LastSeverity_SourceTimestamp = 9045;

        /// <summary>
        /// The identifier for the Rotary_VelocityConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint Rotary_VelocityConditionType_Comment_SourceTimestamp = 9047;

        /// <summary>
        /// The identifier for the Rotary_VelocityConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint Rotary_VelocityConditionType_AddComment_InputArguments = 9052;

        /// <summary>
        /// The identifier for the Rotary_VelocityConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint Rotary_VelocityConditionType_ConditionRefresh_InputArguments = 9054;

        /// <summary>
        /// The identifier for the Rotary_VelocityConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint Rotary_VelocityConditionType_ConditionRefresh2_InputArguments = 10175;

        /// <summary>
        /// The identifier for the Rotary_VelocityConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint Rotary_VelocityConditionType_ActiveState_Id = 9057;

        /// <summary>
        /// The identifier for the Rotary_VelocityConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint Rotary_VelocityConditionType_LimitState_CurrentState = 9066;

        /// <summary>
        /// The identifier for the Rotary_VelocityConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint Rotary_VelocityConditionType_LimitState_CurrentState_Id = 9067;

        /// <summary>
        /// The identifier for the Rotary_VelocityConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint Rotary_VelocityConditionType_LimitState_LastTransition_Id = 9072;

        /// <summary>
        /// The identifier for the Sound_LevelConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint Sound_LevelConditionType_EnabledState_Id = 9095;

        /// <summary>
        /// The identifier for the Sound_LevelConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint Sound_LevelConditionType_Quality_SourceTimestamp = 9104;

        /// <summary>
        /// The identifier for the Sound_LevelConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint Sound_LevelConditionType_LastSeverity_SourceTimestamp = 9106;

        /// <summary>
        /// The identifier for the Sound_LevelConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint Sound_LevelConditionType_Comment_SourceTimestamp = 9108;

        /// <summary>
        /// The identifier for the Sound_LevelConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint Sound_LevelConditionType_AddComment_InputArguments = 9113;

        /// <summary>
        /// The identifier for the Sound_LevelConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint Sound_LevelConditionType_ConditionRefresh_InputArguments = 9115;

        /// <summary>
        /// The identifier for the Sound_LevelConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint Sound_LevelConditionType_ConditionRefresh2_InputArguments = 10177;

        /// <summary>
        /// The identifier for the Sound_LevelConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint Sound_LevelConditionType_ActiveState_Id = 9118;

        /// <summary>
        /// The identifier for the Sound_LevelConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint Sound_LevelConditionType_LimitState_CurrentState = 9127;

        /// <summary>
        /// The identifier for the Sound_LevelConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint Sound_LevelConditionType_LimitState_CurrentState_Id = 9128;

        /// <summary>
        /// The identifier for the Sound_LevelConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint Sound_LevelConditionType_LimitState_LastTransition_Id = 9133;

        /// <summary>
        /// The identifier for the StrainConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint StrainConditionType_EnabledState_Id = 9156;

        /// <summary>
        /// The identifier for the StrainConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint StrainConditionType_Quality_SourceTimestamp = 9165;

        /// <summary>
        /// The identifier for the StrainConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint StrainConditionType_LastSeverity_SourceTimestamp = 9167;

        /// <summary>
        /// The identifier for the StrainConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint StrainConditionType_Comment_SourceTimestamp = 9169;

        /// <summary>
        /// The identifier for the StrainConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint StrainConditionType_AddComment_InputArguments = 9174;

        /// <summary>
        /// The identifier for the StrainConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint StrainConditionType_ConditionRefresh_InputArguments = 9176;

        /// <summary>
        /// The identifier for the StrainConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint StrainConditionType_ConditionRefresh2_InputArguments = 10179;

        /// <summary>
        /// The identifier for the StrainConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint StrainConditionType_ActiveState_Id = 9179;

        /// <summary>
        /// The identifier for the StrainConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint StrainConditionType_LimitState_CurrentState = 9188;

        /// <summary>
        /// The identifier for the StrainConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint StrainConditionType_LimitState_CurrentState_Id = 9189;

        /// <summary>
        /// The identifier for the StrainConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint StrainConditionType_LimitState_LastTransition_Id = 9194;

        /// <summary>
        /// The identifier for the SystemConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint SystemConditionType_EnabledState_Id = 9217;

        /// <summary>
        /// The identifier for the SystemConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint SystemConditionType_Quality_SourceTimestamp = 9226;

        /// <summary>
        /// The identifier for the SystemConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint SystemConditionType_LastSeverity_SourceTimestamp = 9228;

        /// <summary>
        /// The identifier for the SystemConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint SystemConditionType_Comment_SourceTimestamp = 9230;

        /// <summary>
        /// The identifier for the SystemConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint SystemConditionType_AddComment_InputArguments = 9235;

        /// <summary>
        /// The identifier for the SystemConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint SystemConditionType_ConditionRefresh_InputArguments = 9237;

        /// <summary>
        /// The identifier for the SystemConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint SystemConditionType_ConditionRefresh2_InputArguments = 10181;

        /// <summary>
        /// The identifier for the SystemConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint SystemConditionType_ActiveState_Id = 9240;

        /// <summary>
        /// The identifier for the SystemConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint SystemConditionType_LimitState_CurrentState = 9249;

        /// <summary>
        /// The identifier for the SystemConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint SystemConditionType_LimitState_CurrentState_Id = 9250;

        /// <summary>
        /// The identifier for the SystemConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint SystemConditionType_LimitState_LastTransition_Id = 9255;

        /// <summary>
        /// The identifier for the TemperatureConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint TemperatureConditionType_EnabledState_Id = 9278;

        /// <summary>
        /// The identifier for the TemperatureConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint TemperatureConditionType_Quality_SourceTimestamp = 9287;

        /// <summary>
        /// The identifier for the TemperatureConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint TemperatureConditionType_LastSeverity_SourceTimestamp = 9289;

        /// <summary>
        /// The identifier for the TemperatureConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint TemperatureConditionType_Comment_SourceTimestamp = 9291;

        /// <summary>
        /// The identifier for the TemperatureConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint TemperatureConditionType_AddComment_InputArguments = 9296;

        /// <summary>
        /// The identifier for the TemperatureConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint TemperatureConditionType_ConditionRefresh_InputArguments = 9298;

        /// <summary>
        /// The identifier for the TemperatureConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint TemperatureConditionType_ConditionRefresh2_InputArguments = 10183;

        /// <summary>
        /// The identifier for the TemperatureConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint TemperatureConditionType_ActiveState_Id = 9301;

        /// <summary>
        /// The identifier for the TemperatureConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint TemperatureConditionType_LimitState_CurrentState = 9310;

        /// <summary>
        /// The identifier for the TemperatureConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint TemperatureConditionType_LimitState_CurrentState_Id = 9311;

        /// <summary>
        /// The identifier for the TemperatureConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint TemperatureConditionType_LimitState_LastTransition_Id = 9316;

        /// <summary>
        /// The identifier for the TiltConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint TiltConditionType_EnabledState_Id = 9339;

        /// <summary>
        /// The identifier for the TiltConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint TiltConditionType_Quality_SourceTimestamp = 9348;

        /// <summary>
        /// The identifier for the TiltConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint TiltConditionType_LastSeverity_SourceTimestamp = 9350;

        /// <summary>
        /// The identifier for the TiltConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint TiltConditionType_Comment_SourceTimestamp = 9352;

        /// <summary>
        /// The identifier for the TiltConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint TiltConditionType_AddComment_InputArguments = 9357;

        /// <summary>
        /// The identifier for the TiltConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint TiltConditionType_ConditionRefresh_InputArguments = 9359;

        /// <summary>
        /// The identifier for the TiltConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint TiltConditionType_ConditionRefresh2_InputArguments = 10185;

        /// <summary>
        /// The identifier for the TiltConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint TiltConditionType_ActiveState_Id = 9362;

        /// <summary>
        /// The identifier for the TiltConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint TiltConditionType_LimitState_CurrentState = 9371;

        /// <summary>
        /// The identifier for the TiltConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint TiltConditionType_LimitState_CurrentState_Id = 9372;

        /// <summary>
        /// The identifier for the TiltConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint TiltConditionType_LimitState_LastTransition_Id = 9377;

        /// <summary>
        /// The identifier for the TorqueConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint TorqueConditionType_EnabledState_Id = 9400;

        /// <summary>
        /// The identifier for the TorqueConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint TorqueConditionType_Quality_SourceTimestamp = 9409;

        /// <summary>
        /// The identifier for the TorqueConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint TorqueConditionType_LastSeverity_SourceTimestamp = 9411;

        /// <summary>
        /// The identifier for the TorqueConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint TorqueConditionType_Comment_SourceTimestamp = 9413;

        /// <summary>
        /// The identifier for the TorqueConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint TorqueConditionType_AddComment_InputArguments = 9418;

        /// <summary>
        /// The identifier for the TorqueConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint TorqueConditionType_ConditionRefresh_InputArguments = 9420;

        /// <summary>
        /// The identifier for the TorqueConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint TorqueConditionType_ConditionRefresh2_InputArguments = 10187;

        /// <summary>
        /// The identifier for the TorqueConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint TorqueConditionType_ActiveState_Id = 9423;

        /// <summary>
        /// The identifier for the TorqueConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint TorqueConditionType_LimitState_CurrentState = 9432;

        /// <summary>
        /// The identifier for the TorqueConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint TorqueConditionType_LimitState_CurrentState_Id = 9433;

        /// <summary>
        /// The identifier for the TorqueConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint TorqueConditionType_LimitState_LastTransition_Id = 9438;

        /// <summary>
        /// The identifier for the VelocityConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint VelocityConditionType_EnabledState_Id = 9461;

        /// <summary>
        /// The identifier for the VelocityConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint VelocityConditionType_Quality_SourceTimestamp = 9470;

        /// <summary>
        /// The identifier for the VelocityConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint VelocityConditionType_LastSeverity_SourceTimestamp = 9472;

        /// <summary>
        /// The identifier for the VelocityConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint VelocityConditionType_Comment_SourceTimestamp = 9474;

        /// <summary>
        /// The identifier for the VelocityConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint VelocityConditionType_AddComment_InputArguments = 9479;

        /// <summary>
        /// The identifier for the VelocityConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint VelocityConditionType_ConditionRefresh_InputArguments = 9481;

        /// <summary>
        /// The identifier for the VelocityConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint VelocityConditionType_ConditionRefresh2_InputArguments = 10189;

        /// <summary>
        /// The identifier for the VelocityConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint VelocityConditionType_ActiveState_Id = 9484;

        /// <summary>
        /// The identifier for the VelocityConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint VelocityConditionType_LimitState_CurrentState = 9493;

        /// <summary>
        /// The identifier for the VelocityConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint VelocityConditionType_LimitState_CurrentState_Id = 9494;

        /// <summary>
        /// The identifier for the VelocityConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint VelocityConditionType_LimitState_LastTransition_Id = 9499;

        /// <summary>
        /// The identifier for the ViscosityConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint ViscosityConditionType_EnabledState_Id = 9522;

        /// <summary>
        /// The identifier for the ViscosityConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint ViscosityConditionType_Quality_SourceTimestamp = 9531;

        /// <summary>
        /// The identifier for the ViscosityConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint ViscosityConditionType_LastSeverity_SourceTimestamp = 9533;

        /// <summary>
        /// The identifier for the ViscosityConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint ViscosityConditionType_Comment_SourceTimestamp = 9535;

        /// <summary>
        /// The identifier for the ViscosityConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint ViscosityConditionType_AddComment_InputArguments = 9540;

        /// <summary>
        /// The identifier for the ViscosityConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint ViscosityConditionType_ConditionRefresh_InputArguments = 9542;

        /// <summary>
        /// The identifier for the ViscosityConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint ViscosityConditionType_ConditionRefresh2_InputArguments = 10191;

        /// <summary>
        /// The identifier for the ViscosityConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint ViscosityConditionType_ActiveState_Id = 9545;

        /// <summary>
        /// The identifier for the ViscosityConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint ViscosityConditionType_LimitState_CurrentState = 9554;

        /// <summary>
        /// The identifier for the ViscosityConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint ViscosityConditionType_LimitState_CurrentState_Id = 9555;

        /// <summary>
        /// The identifier for the ViscosityConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint ViscosityConditionType_LimitState_LastTransition_Id = 9560;

        /// <summary>
        /// The identifier for the VoltageConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint VoltageConditionType_EnabledState_Id = 9583;

        /// <summary>
        /// The identifier for the VoltageConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint VoltageConditionType_Quality_SourceTimestamp = 9592;

        /// <summary>
        /// The identifier for the VoltageConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint VoltageConditionType_LastSeverity_SourceTimestamp = 9594;

        /// <summary>
        /// The identifier for the VoltageConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint VoltageConditionType_Comment_SourceTimestamp = 9596;

        /// <summary>
        /// The identifier for the VoltageConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint VoltageConditionType_AddComment_InputArguments = 9601;

        /// <summary>
        /// The identifier for the VoltageConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint VoltageConditionType_ConditionRefresh_InputArguments = 9603;

        /// <summary>
        /// The identifier for the VoltageConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint VoltageConditionType_ConditionRefresh2_InputArguments = 10193;

        /// <summary>
        /// The identifier for the VoltageConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint VoltageConditionType_ActiveState_Id = 9606;

        /// <summary>
        /// The identifier for the VoltageConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint VoltageConditionType_LimitState_CurrentState = 9615;

        /// <summary>
        /// The identifier for the VoltageConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint VoltageConditionType_LimitState_CurrentState_Id = 9616;

        /// <summary>
        /// The identifier for the VoltageConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint VoltageConditionType_LimitState_LastTransition_Id = 9621;

        /// <summary>
        /// The identifier for the Volt_AmperageConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint Volt_AmperageConditionType_EnabledState_Id = 9644;

        /// <summary>
        /// The identifier for the Volt_AmperageConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint Volt_AmperageConditionType_Quality_SourceTimestamp = 9653;

        /// <summary>
        /// The identifier for the Volt_AmperageConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint Volt_AmperageConditionType_LastSeverity_SourceTimestamp = 9655;

        /// <summary>
        /// The identifier for the Volt_AmperageConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint Volt_AmperageConditionType_Comment_SourceTimestamp = 9657;

        /// <summary>
        /// The identifier for the Volt_AmperageConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint Volt_AmperageConditionType_AddComment_InputArguments = 9662;

        /// <summary>
        /// The identifier for the Volt_AmperageConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint Volt_AmperageConditionType_ConditionRefresh_InputArguments = 9664;

        /// <summary>
        /// The identifier for the Volt_AmperageConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint Volt_AmperageConditionType_ConditionRefresh2_InputArguments = 10195;

        /// <summary>
        /// The identifier for the Volt_AmperageConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint Volt_AmperageConditionType_ActiveState_Id = 9667;

        /// <summary>
        /// The identifier for the Volt_AmperageConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint Volt_AmperageConditionType_LimitState_CurrentState = 9676;

        /// <summary>
        /// The identifier for the Volt_AmperageConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint Volt_AmperageConditionType_LimitState_CurrentState_Id = 9677;

        /// <summary>
        /// The identifier for the Volt_AmperageConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint Volt_AmperageConditionType_LimitState_LastTransition_Id = 9682;

        /// <summary>
        /// The identifier for the VoltAmperageReactiveConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint VoltAmperageReactiveConditionType_EnabledState_Id = 9705;

        /// <summary>
        /// The identifier for the VoltAmperageReactiveConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint VoltAmperageReactiveConditionType_Quality_SourceTimestamp = 9714;

        /// <summary>
        /// The identifier for the VoltAmperageReactiveConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint VoltAmperageReactiveConditionType_LastSeverity_SourceTimestamp = 9716;

        /// <summary>
        /// The identifier for the VoltAmperageReactiveConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint VoltAmperageReactiveConditionType_Comment_SourceTimestamp = 9718;

        /// <summary>
        /// The identifier for the VoltAmperageReactiveConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint VoltAmperageReactiveConditionType_AddComment_InputArguments = 9723;

        /// <summary>
        /// The identifier for the VoltAmperageReactiveConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint VoltAmperageReactiveConditionType_ConditionRefresh_InputArguments = 9725;

        /// <summary>
        /// The identifier for the VoltAmperageReactiveConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint VoltAmperageReactiveConditionType_ConditionRefresh2_InputArguments = 10197;

        /// <summary>
        /// The identifier for the VoltAmperageReactiveConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint VoltAmperageReactiveConditionType_ActiveState_Id = 9728;

        /// <summary>
        /// The identifier for the VoltAmperageReactiveConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint VoltAmperageReactiveConditionType_LimitState_CurrentState = 9737;

        /// <summary>
        /// The identifier for the VoltAmperageReactiveConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint VoltAmperageReactiveConditionType_LimitState_CurrentState_Id = 9738;

        /// <summary>
        /// The identifier for the VoltAmperageReactiveConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint VoltAmperageReactiveConditionType_LimitState_LastTransition_Id = 9743;

        /// <summary>
        /// The identifier for the WattageConditionType_EnabledState_Id Variable.
        /// </summary>
        public const uint WattageConditionType_EnabledState_Id = 9766;

        /// <summary>
        /// The identifier for the WattageConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint WattageConditionType_Quality_SourceTimestamp = 9775;

        /// <summary>
        /// The identifier for the WattageConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint WattageConditionType_LastSeverity_SourceTimestamp = 9777;

        /// <summary>
        /// The identifier for the WattageConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint WattageConditionType_Comment_SourceTimestamp = 9779;

        /// <summary>
        /// The identifier for the WattageConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint WattageConditionType_AddComment_InputArguments = 9784;

        /// <summary>
        /// The identifier for the WattageConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint WattageConditionType_ConditionRefresh_InputArguments = 9786;

        /// <summary>
        /// The identifier for the WattageConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint WattageConditionType_ConditionRefresh2_InputArguments = 10199;

        /// <summary>
        /// The identifier for the WattageConditionType_ActiveState_Id Variable.
        /// </summary>
        public const uint WattageConditionType_ActiveState_Id = 9789;

        /// <summary>
        /// The identifier for the WattageConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public const uint WattageConditionType_LimitState_CurrentState = 9798;

        /// <summary>
        /// The identifier for the WattageConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public const uint WattageConditionType_LimitState_CurrentState_Id = 9799;

        /// <summary>
        /// The identifier for the WattageConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public const uint WattageConditionType_LimitState_LastTransition_Id = 9804;

        /// <summary>
        /// The identifier for the ChannelType_CalibrationDate Variable.
        /// </summary>
        public const uint ChannelType_CalibrationDate = 9812;

        /// <summary>
        /// The identifier for the ChannelType_NextCalibrationDate Variable.
        /// </summary>
        public const uint ChannelType_NextCalibrationDate = 9813;

        /// <summary>
        /// The identifier for the ChannelType_CalibrationInitials Variable.
        /// </summary>
        public const uint ChannelType_CalibrationInitials = 9814;

        /// <summary>
        /// The identifier for the SensorConfigurationType_FirmwareVersion Variable.
        /// </summary>
        public const uint SensorConfigurationType_FirmwareVersion = 9841;

        /// <summary>
        /// The identifier for the SourceType_ComponentId Variable.
        /// </summary>
        public const uint SourceType_ComponentId = 9868;

        /// <summary>
        /// The identifier for the AssetType_SerialNumber Variable.
        /// </summary>
        public const uint AssetType_SerialNumber = 9870;

        /// <summary>
        /// The identifier for the AssetType_Manufacturers Variable.
        /// </summary>
        public const uint AssetType_Manufacturers = 9871;

        /// <summary>
        /// The identifier for the AssetType_LastChangeTimestamp Variable.
        /// </summary>
        public const uint AssetType_LastChangeTimestamp = 9872;

        /// <summary>
        /// The identifier for the AssetType_AssetDescription Variable.
        /// </summary>
        public const uint AssetType_AssetDescription = 9873;

        /// <summary>
        /// The identifier for the MeasurementType_SignificantDigits Variable.
        /// </summary>
        public const uint MeasurementType_SignificantDigits = 9880;

        /// <summary>
        /// The identifier for the MeasurementType_Units Variable.
        /// </summary>
        public const uint MeasurementType_Units = 9881;

        /// <summary>
        /// The identifier for the MeasurementType_NativeUnits Variable.
        /// </summary>
        public const uint MeasurementType_NativeUnits = 9882;

        /// <summary>
        /// The identifier for the MeasurementType_LastChangeTimestamp Variable.
        /// </summary>
        public const uint MeasurementType_LastChangeTimestamp = 9883;

        /// <summary>
        /// The identifier for the MeasurementType_Code Variable.
        /// </summary>
        public const uint MeasurementType_Code = 9884;

        /// <summary>
        /// The identifier for the MeasurementType_Maximum Variable.
        /// </summary>
        public const uint MeasurementType_Maximum = 9885;

        /// <summary>
        /// The identifier for the MeasurementType_Minimum Variable.
        /// </summary>
        public const uint MeasurementType_Minimum = 9886;

        /// <summary>
        /// The identifier for the MeasurementType_Nominal Variable.
        /// </summary>
        public const uint MeasurementType_Nominal = 9887;

        /// <summary>
        /// The identifier for the CuttingToolType_ToolId Variable.
        /// </summary>
        public const uint CuttingToolType_ToolId = 9889;

        /// <summary>
        /// The identifier for the CuttingToolType_ToolGroup Variable.
        /// </summary>
        public const uint CuttingToolType_ToolGroup = 9890;

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_CutterStatus Variable.
        /// </summary>
        public const uint CuttingToolLifeCycleType_CutterStatus = 9896;

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_CutterStatus_Status Variable.
        /// </summary>
        public const uint CuttingToolLifeCycleType_CutterStatus_Status = 9897;

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_ReconditionCount Variable.
        /// </summary>
        public const uint CuttingToolLifeCycleType_ReconditionCount = 9898;

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_ToolLife Variable.
        /// </summary>
        public const uint CuttingToolLifeCycleType_ToolLife = 9900;

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_ToolLife_Type Variable.
        /// </summary>
        public const uint CuttingToolLifeCycleType_ToolLife_Type = 9901;

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_ToolLife_CountDirection Variable.
        /// </summary>
        public const uint CuttingToolLifeCycleType_ToolLife_CountDirection = 9902;

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_ToolLife_Maximum Variable.
        /// </summary>
        public const uint CuttingToolLifeCycleType_ToolLife_Maximum = 9904;

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_ProgramToolNumber Variable.
        /// </summary>
        public const uint CuttingToolLifeCycleType_ProgramToolNumber = 9905;

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_ProgramToolNumber_Maximum Variable.
        /// </summary>
        public const uint CuttingToolLifeCycleType_ProgramToolNumber_Maximum = 9906;

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_ProgramToolNumber_Minimum Variable.
        /// </summary>
        public const uint CuttingToolLifeCycleType_ProgramToolNumber_Minimum = 9907;

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_ProgramToolNumber_Nominal Variable.
        /// </summary>
        public const uint CuttingToolLifeCycleType_ProgramToolNumber_Nominal = 9908;

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_Location Variable.
        /// </summary>
        public const uint CuttingToolLifeCycleType_Location = 9909;

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_Location_Type Variable.
        /// </summary>
        public const uint CuttingToolLifeCycleType_Location_Type = 9910;

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_Location_NegativeOverlap Variable.
        /// </summary>
        public const uint CuttingToolLifeCycleType_Location_NegativeOverlap = 9911;

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_Location_PositiveOverlap Variable.
        /// </summary>
        public const uint CuttingToolLifeCycleType_Location_PositiveOverlap = 9912;

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_ProgramSpindleSpeed Variable.
        /// </summary>
        public const uint CuttingToolLifeCycleType_ProgramSpindleSpeed = 9913;

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_ProgramSpindleSpeed_Maximum Variable.
        /// </summary>
        public const uint CuttingToolLifeCycleType_ProgramSpindleSpeed_Maximum = 9914;

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_ProgramSpindleSpeed_Minimum Variable.
        /// </summary>
        public const uint CuttingToolLifeCycleType_ProgramSpindleSpeed_Minimum = 9915;

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_ProgramSpindleSpeed_Nominal Variable.
        /// </summary>
        public const uint CuttingToolLifeCycleType_ProgramSpindleSpeed_Nominal = 9916;

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_ProgramFeedRate Variable.
        /// </summary>
        public const uint CuttingToolLifeCycleType_ProgramFeedRate = 9917;

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_ConnectionCodeMachineSide Variable.
        /// </summary>
        public const uint CuttingToolLifeCycleType_ConnectionCodeMachineSide = 9918;

        /// <summary>
        /// The identifier for the CutterStatusType_Status Variable.
        /// </summary>
        public const uint CutterStatusType_Status = 9920;

        /// <summary>
        /// The identifier for the ReconditionCountType_MaximumCount Variable.
        /// </summary>
        public const uint ReconditionCountType_MaximumCount = 9922;

        /// <summary>
        /// The identifier for the LifeType_Type Variable.
        /// </summary>
        public const uint LifeType_Type = 9924;

        /// <summary>
        /// The identifier for the LifeType_CountDirection Variable.
        /// </summary>
        public const uint LifeType_CountDirection = 9925;

        /// <summary>
        /// The identifier for the LifeType_WarningLevel Variable.
        /// </summary>
        public const uint LifeType_WarningLevel = 9926;

        /// <summary>
        /// The identifier for the LifeType_Maximum Variable.
        /// </summary>
        public const uint LifeType_Maximum = 9927;

        /// <summary>
        /// The identifier for the LocationType_Type Variable.
        /// </summary>
        public const uint LocationType_Type = 9929;

        /// <summary>
        /// The identifier for the LocationType_NegativeOverlap Variable.
        /// </summary>
        public const uint LocationType_NegativeOverlap = 9930;

        /// <summary>
        /// The identifier for the LocationType_PositiveOverlap Variable.
        /// </summary>
        public const uint LocationType_PositiveOverlap = 9931;

        /// <summary>
        /// The identifier for the ProgramSpindleSpeedType_Maximum Variable.
        /// </summary>
        public const uint ProgramSpindleSpeedType_Maximum = 9933;

        /// <summary>
        /// The identifier for the ProgramSpindleSpeedType_Minimum Variable.
        /// </summary>
        public const uint ProgramSpindleSpeedType_Minimum = 9934;

        /// <summary>
        /// The identifier for the ProgramSpindleSpeedType_Nominal Variable.
        /// </summary>
        public const uint ProgramSpindleSpeedType_Nominal = 9935;

        /// <summary>
        /// The identifier for the ProgramFeedRateType_Maximum Variable.
        /// </summary>
        public const uint ProgramFeedRateType_Maximum = 9937;

        /// <summary>
        /// The identifier for the ProgramFeedRateType_Minimum Variable.
        /// </summary>
        public const uint ProgramFeedRateType_Minimum = 9938;

        /// <summary>
        /// The identifier for the ProgramFeedRateType_Nominal Variable.
        /// </summary>
        public const uint ProgramFeedRateType_Nominal = 9939;

        /// <summary>
        /// The identifier for the CuttingItemType_Indices Variable.
        /// </summary>
        public const uint CuttingItemType_Indices = 9941;

        /// <summary>
        /// The identifier for the CuttingItemType_ItemId Variable.
        /// </summary>
        public const uint CuttingItemType_ItemId = 9942;

        /// <summary>
        /// The identifier for the CuttingItemType_Grade Variable.
        /// </summary>
        public const uint CuttingItemType_Grade = 9943;

        /// <summary>
        /// The identifier for the CuttingItemType_Manufacturers Variable.
        /// </summary>
        public const uint CuttingItemType_Manufacturers = 9944;

        /// <summary>
        /// The identifier for the CuttingItemType_Description Variable.
        /// </summary>
        public const uint CuttingItemType_Description = 9945;

        /// <summary>
        /// The identifier for the CuttingItemType_Locus Variable.
        /// </summary>
        public const uint CuttingItemType_Locus = 9946;

        /// <summary>
        /// The identifier for the ActuatorStateTypeEnum_EnumValues Variable.
        /// </summary>
        public const uint ActuatorStateTypeEnum_EnumValues = 9964;

        /// <summary>
        /// The identifier for the AlarmStateTypeEnum_EnumValues Variable.
        /// </summary>
        public const uint AlarmStateTypeEnum_EnumValues = 9966;

        /// <summary>
        /// The identifier for the AvailabilityTypeEnum_EnumValues Variable.
        /// </summary>
        public const uint AvailabilityTypeEnum_EnumValues = 9972;

        /// <summary>
        /// The identifier for the AxesCouplingTypeEnum_EnumValues Variable.
        /// </summary>
        public const uint AxesCouplingTypeEnum_EnumValues = 9974;

        /// <summary>
        /// The identifier for the CategoryTypeEnum_EnumValues Variable.
        /// </summary>
        public const uint CategoryTypeEnum_EnumValues = 9980;

        /// <summary>
        /// The identifier for the ClampStateTypeEnum_EnumValues Variable.
        /// </summary>
        public const uint ClampStateTypeEnum_EnumValues = 9983;

        /// <summary>
        /// The identifier for the ControllerModeTypeEnum_EnumValues Variable.
        /// </summary>
        public const uint ControllerModeTypeEnum_EnumValues = 9986;

        /// <summary>
        /// The identifier for the CoordinateSystemTypeEnum_EnumValues Variable.
        /// </summary>
        public const uint CoordinateSystemTypeEnum_EnumValues = 9988;

        /// <summary>
        /// The identifier for the CutterStatusValueTypeEnum_EnumValues Variable.
        /// </summary>
        public const uint CutterStatusValueTypeEnum_EnumValues = 9996;

        /// <summary>
        /// The identifier for the DataItemEnumTypeEnum_EnumValues Variable.
        /// </summary>
        public const uint DataItemEnumTypeEnum_EnumValues = 9999;

        /// <summary>
        /// The identifier for the DataItemStatisticsTypeEnum_EnumValues Variable.
        /// </summary>
        public const uint DataItemStatisticsTypeEnum_EnumValues = 10003;

        /// <summary>
        /// The identifier for the DataItemSubEnumTypeEnum_EnumValues Variable.
        /// </summary>
        public const uint DataItemSubEnumTypeEnum_EnumValues = 10005;

        /// <summary>
        /// The identifier for the DefinitionFormatTypeEnum_EnumValues Variable.
        /// </summary>
        public const uint DefinitionFormatTypeEnum_EnumValues = 10007;

        /// <summary>
        /// The identifier for the DirectionTypeEnum_EnumValues Variable.
        /// </summary>
        public const uint DirectionTypeEnum_EnumValues = 10011;

        /// <summary>
        /// The identifier for the DoorStateTypeEnum_EnumValues Variable.
        /// </summary>
        public const uint DoorStateTypeEnum_EnumValues = 10013;

        /// <summary>
        /// The identifier for the EmergencyStopTypeEnum_EnumValues Variable.
        /// </summary>
        public const uint EmergencyStopTypeEnum_EnumValues = 10018;

        /// <summary>
        /// The identifier for the ExecutionTypeEnum_EnumValues Variable.
        /// </summary>
        public const uint ExecutionTypeEnum_EnumValues = 10021;

        /// <summary>
        /// The identifier for the InterfaceStateTypeEnum_EnumValues Variable.
        /// </summary>
        public const uint InterfaceStateTypeEnum_EnumValues = 10030;

        /// <summary>
        /// The identifier for the LocationsTypeEnum_EnumValues Variable.
        /// </summary>
        public const uint LocationsTypeEnum_EnumValues = 10035;

        /// <summary>
        /// The identifier for the NotificationCodeTypeEnum_EnumValues Variable.
        /// </summary>
        public const uint NotificationCodeTypeEnum_EnumValues = 10052;

        /// <summary>
        /// The identifier for the PathModeTypeEnum_EnumValues Variable.
        /// </summary>
        public const uint PathModeTypeEnum_EnumValues = 10057;

        /// <summary>
        /// The identifier for the PowerStateTypeEnum_EnumValues Variable.
        /// </summary>
        public const uint PowerStateTypeEnum_EnumValues = 10059;

        /// <summary>
        /// The identifier for the QualifierTypeEnum_EnumValues Variable.
        /// </summary>
        public const uint QualifierTypeEnum_EnumValues = 10062;

        /// <summary>
        /// The identifier for the RotaryModeTypeEnum_EnumValues Variable.
        /// </summary>
        public const uint RotaryModeTypeEnum_EnumValues = 10066;

        /// <summary>
        /// The identifier for the SeverityTypeEnum_EnumValues Variable.
        /// </summary>
        public const uint SeverityTypeEnum_EnumValues = 10069;

        /// <summary>
        /// The identifier for the ToolLifeDirectionTypeEnum_EnumValues Variable.
        /// </summary>
        public const uint ToolLifeDirectionTypeEnum_EnumValues = 10081;

        /// <summary>
        /// The identifier for the ToolLifeTypeEnum_EnumValues Variable.
        /// </summary>
        public const uint ToolLifeTypeEnum_EnumValues = 10083;

        /// <summary>
        /// The identifier for the OpcUaMTConnect_BinarySchema Variable.
        /// </summary>
        public const uint OpcUaMTConnect_BinarySchema = 10101;

        /// <summary>
        /// The identifier for the OpcUaMTConnect_BinarySchema_NamespaceUri Variable.
        /// </summary>
        public const uint OpcUaMTConnect_BinarySchema_NamespaceUri = 10103;

        /// <summary>
        /// The identifier for the OpcUaMTConnect_BinarySchema_Deprecated Variable.
        /// </summary>
        public const uint OpcUaMTConnect_BinarySchema_Deprecated = 10202;

        /// <summary>
        /// The identifier for the OpcUaMTConnect_BinarySchema_DataItemEnumDataType Variable.
        /// </summary>
        public const uint OpcUaMTConnect_BinarySchema_DataItemEnumDataType = 10104;

        /// <summary>
        /// The identifier for the OpcUaMTConnect_BinarySchema_DataItemStatisticsDataType Variable.
        /// </summary>
        public const uint OpcUaMTConnect_BinarySchema_DataItemStatisticsDataType = 10107;

        /// <summary>
        /// The identifier for the OpcUaMTConnect_XmlSchema Variable.
        /// </summary>
        public const uint OpcUaMTConnect_XmlSchema = 10090;

        /// <summary>
        /// The identifier for the OpcUaMTConnect_XmlSchema_NamespaceUri Variable.
        /// </summary>
        public const uint OpcUaMTConnect_XmlSchema_NamespaceUri = 10092;

        /// <summary>
        /// The identifier for the OpcUaMTConnect_XmlSchema_Deprecated Variable.
        /// </summary>
        public const uint OpcUaMTConnect_XmlSchema_Deprecated = 10204;

        /// <summary>
        /// The identifier for the OpcUaMTConnect_XmlSchema_DataItemEnumDataType Variable.
        /// </summary>
        public const uint OpcUaMTConnect_XmlSchema_DataItemEnumDataType = 10093;

        /// <summary>
        /// The identifier for the OpcUaMTConnect_XmlSchema_DataItemStatisticsDataType Variable.
        /// </summary>
        public const uint OpcUaMTConnect_XmlSchema_DataItemStatisticsDataType = 10096;
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
        /// The identifier for the MTSampleDataItemType VariableType.
        /// </summary>
        public const uint MTSampleDataItemType = 6694;

        /// <summary>
        /// The identifier for the AccelerationType VariableType.
        /// </summary>
        public const uint AccelerationType = 6704;

        /// <summary>
        /// The identifier for the AccumulatedTimeType VariableType.
        /// </summary>
        public const uint AccumulatedTimeType = 6714;

        /// <summary>
        /// The identifier for the AmperageType VariableType.
        /// </summary>
        public const uint AmperageType = 6724;

        /// <summary>
        /// The identifier for the AngleType VariableType.
        /// </summary>
        public const uint AngleType = 6734;

        /// <summary>
        /// The identifier for the AngularAccelerationType VariableType.
        /// </summary>
        public const uint AngularAccelerationType = 6744;

        /// <summary>
        /// The identifier for the AngularVelocityType VariableType.
        /// </summary>
        public const uint AngularVelocityType = 6754;

        /// <summary>
        /// The identifier for the AxisFeedrateType VariableType.
        /// </summary>
        public const uint AxisFeedrateType = 6764;

        /// <summary>
        /// The identifier for the ConcentrationType VariableType.
        /// </summary>
        public const uint ConcentrationType = 6774;

        /// <summary>
        /// The identifier for the ConductivityType VariableType.
        /// </summary>
        public const uint ConductivityType = 6784;

        /// <summary>
        /// The identifier for the DisplacementType VariableType.
        /// </summary>
        public const uint DisplacementType = 6794;

        /// <summary>
        /// The identifier for the ElectricalEnergyType VariableType.
        /// </summary>
        public const uint ElectricalEnergyType = 6804;

        /// <summary>
        /// The identifier for the FillLevelType VariableType.
        /// </summary>
        public const uint FillLevelType = 6814;

        /// <summary>
        /// The identifier for the FlowType VariableType.
        /// </summary>
        public const uint FlowType = 6824;

        /// <summary>
        /// The identifier for the FrequencyType VariableType.
        /// </summary>
        public const uint FrequencyType = 6834;

        /// <summary>
        /// The identifier for the LinearForceType VariableType.
        /// </summary>
        public const uint LinearForceType = 6844;

        /// <summary>
        /// The identifier for the LoadType VariableType.
        /// </summary>
        public const uint LoadType = 6854;

        /// <summary>
        /// The identifier for the MassType VariableType.
        /// </summary>
        public const uint MassType = 6864;

        /// <summary>
        /// The identifier for the PathFeedrateType VariableType.
        /// </summary>
        public const uint PathFeedrateType = 6874;

        /// <summary>
        /// The identifier for the PathPositionType VariableType.
        /// </summary>
        public const uint PathPositionType = 6884;

        /// <summary>
        /// The identifier for the PHType VariableType.
        /// </summary>
        public const uint PHType = 6894;

        /// <summary>
        /// The identifier for the PositionType VariableType.
        /// </summary>
        public const uint PositionType = 6904;

        /// <summary>
        /// The identifier for the PowerFactorType VariableType.
        /// </summary>
        public const uint PowerFactorType = 6914;

        /// <summary>
        /// The identifier for the PressureType VariableType.
        /// </summary>
        public const uint PressureType = 6924;

        /// <summary>
        /// The identifier for the ResistanceType VariableType.
        /// </summary>
        public const uint ResistanceType = 6934;

        /// <summary>
        /// The identifier for the RotationalVelocityType VariableType.
        /// </summary>
        public const uint RotationalVelocityType = 6944;

        /// <summary>
        /// The identifier for the SoundPressureType VariableType.
        /// </summary>
        public const uint SoundPressureType = 6954;

        /// <summary>
        /// The identifier for the StrainType VariableType.
        /// </summary>
        public const uint StrainType = 6964;

        /// <summary>
        /// The identifier for the TemperatureType VariableType.
        /// </summary>
        public const uint TemperatureType = 6974;

        /// <summary>
        /// The identifier for the TiltType VariableType.
        /// </summary>
        public const uint TiltType = 6984;

        /// <summary>
        /// The identifier for the TorqueType VariableType.
        /// </summary>
        public const uint TorqueType = 6994;

        /// <summary>
        /// The identifier for the VelocityType VariableType.
        /// </summary>
        public const uint VelocityType = 7004;

        /// <summary>
        /// The identifier for the ViscosityType VariableType.
        /// </summary>
        public const uint ViscosityType = 7014;

        /// <summary>
        /// The identifier for the VoltageType VariableType.
        /// </summary>
        public const uint VoltageType = 7024;

        /// <summary>
        /// The identifier for the VoltAmpereType VariableType.
        /// </summary>
        public const uint VoltAmpereType = 7034;

        /// <summary>
        /// The identifier for the VoltAmpereReactiveType VariableType.
        /// </summary>
        public const uint VoltAmpereReactiveType = 7044;

        /// <summary>
        /// The identifier for the WattageType VariableType.
        /// </summary>
        public const uint WattageType = 7054;

        /// <summary>
        /// The identifier for the MTEventDataItemType VariableType.
        /// </summary>
        public const uint MTEventDataItemType = 7064;

        /// <summary>
        /// The identifier for the ActiveAxesType VariableType.
        /// </summary>
        public const uint ActiveAxesType = 7069;

        /// <summary>
        /// The identifier for the ActuatorStateType VariableType.
        /// </summary>
        public const uint ActuatorStateType = 7074;

        /// <summary>
        /// The identifier for the AvailabilityType VariableType.
        /// </summary>
        public const uint AvailabilityType = 7079;

        /// <summary>
        /// The identifier for the AxesCouplingType VariableType.
        /// </summary>
        public const uint AxesCouplingType = 7084;

        /// <summary>
        /// The identifier for the BlockType VariableType.
        /// </summary>
        public const uint BlockType = 1003;

        /// <summary>
        /// The identifier for the ClampStateType VariableType.
        /// </summary>
        public const uint ClampStateType = 7093;

        /// <summary>
        /// The identifier for the ControllerModeType VariableType.
        /// </summary>
        public const uint ControllerModeType = 7098;

        /// <summary>
        /// The identifier for the CoupledAxesType VariableType.
        /// </summary>
        public const uint CoupledAxesType = 7103;

        /// <summary>
        /// The identifier for the DirectionType VariableType.
        /// </summary>
        public const uint DirectionType = 7108;

        /// <summary>
        /// The identifier for the DoorStateType VariableType.
        /// </summary>
        public const uint DoorStateType = 7113;

        /// <summary>
        /// The identifier for the EmergencyStopType VariableType.
        /// </summary>
        public const uint EmergencyStopType = 7118;

        /// <summary>
        /// The identifier for the ExecutionType VariableType.
        /// </summary>
        public const uint ExecutionType = 7123;

        /// <summary>
        /// The identifier for the LineType VariableType.
        /// </summary>
        public const uint LineType = 7128;

        /// <summary>
        /// The identifier for the MessageType VariableType.
        /// </summary>
        public const uint MessageType = 7133;

        /// <summary>
        /// The identifier for the PalletIdType VariableType.
        /// </summary>
        public const uint PalletIdType = 7138;

        /// <summary>
        /// The identifier for the PartCountType VariableType.
        /// </summary>
        public const uint PartCountType = 7143;

        /// <summary>
        /// The identifier for the PartIdType VariableType.
        /// </summary>
        public const uint PartIdType = 7148;

        /// <summary>
        /// The identifier for the PathModeType VariableType.
        /// </summary>
        public const uint PathModeType = 7153;

        /// <summary>
        /// The identifier for the PowerStateType VariableType.
        /// </summary>
        public const uint PowerStateType = 7158;

        /// <summary>
        /// The identifier for the ProgramType VariableType.
        /// </summary>
        public const uint ProgramType = 7163;

        /// <summary>
        /// The identifier for the RotaryModeType VariableType.
        /// </summary>
        public const uint RotaryModeType = 7168;

        /// <summary>
        /// The identifier for the ToolAssetIdType VariableType.
        /// </summary>
        public const uint ToolAssetIdType = 7173;

        /// <summary>
        /// The identifier for the ToolNumberType VariableType.
        /// </summary>
        public const uint ToolNumberType = 7178;

        /// <summary>
        /// The identifier for the WorkHoldingIdType VariableType.
        /// </summary>
        public const uint WorkHoldingIdType = 7183;

        /// <summary>
        /// The identifier for the ChannelType VariableType.
        /// </summary>
        public const uint ChannelType = 9811;

        /// <summary>
        /// The identifier for the MeasurementType VariableType.
        /// </summary>
        public const uint MeasurementType = 9874;

        /// <summary>
        /// The identifier for the CutterStatusType VariableType.
        /// </summary>
        public const uint CutterStatusType = 9919;

        /// <summary>
        /// The identifier for the ReconditionCountType VariableType.
        /// </summary>
        public const uint ReconditionCountType = 9921;

        /// <summary>
        /// The identifier for the LifeType VariableType.
        /// </summary>
        public const uint LifeType = 9923;

        /// <summary>
        /// The identifier for the LocationType VariableType.
        /// </summary>
        public const uint LocationType = 9928;

        /// <summary>
        /// The identifier for the ProgramSpindleSpeedType VariableType.
        /// </summary>
        public const uint ProgramSpindleSpeedType = 9932;

        /// <summary>
        /// The identifier for the ProgramFeedRateType VariableType.
        /// </summary>
        public const uint ProgramFeedRateType = 9936;

        /// <summary>
        /// The identifier for the CuttingItemType VariableType.
        /// </summary>
        public const uint CuttingItemType = 9940;

        /// <summary>
        /// The identifier for the AssemblyMeasurementType VariableType.
        /// </summary>
        public const uint AssemblyMeasurementType = 9949;
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
        /// The identifier for the ActuatorStateTypeEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId ActuatorStateTypeEnum = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.ActuatorStateTypeEnum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AlarmStateTypeEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId AlarmStateTypeEnum = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.AlarmStateTypeEnum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AssetDescriptionType DataType.
        /// </summary>
        public static readonly ExpandedNodeId AssetDescriptionType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.AssetDescriptionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AssetAttrDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId AssetAttrDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.AssetAttrDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AssetBufferSizeDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId AssetBufferSizeDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.AssetBufferSizeDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AssetIdDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId AssetIdDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.AssetIdDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AvailabilityTypeEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId AvailabilityTypeEnum = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.AvailabilityTypeEnum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AxesCouplingTypeEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId AxesCouplingTypeEnum = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.AxesCouplingTypeEnum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AxesListValueDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId AxesListValueDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.AxesListValueDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the BufferSizeDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId BufferSizeDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.BufferSizeDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CalibrationDateDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId CalibrationDateDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.CalibrationDateDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CalibrationInitialsDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId CalibrationInitialsDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.CalibrationInitialsDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CategoryTypeEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId CategoryTypeEnum = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.CategoryTypeEnum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ChannelNumberDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId ChannelNumberDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.ChannelNumberDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ClampStateTypeEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId ClampStateTypeEnum = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.ClampStateTypeEnum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ConditionDescriptionDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId ConditionDescriptionDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.ConditionDescriptionDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ControllerModeTypeEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId ControllerModeTypeEnum = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.ControllerModeTypeEnum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CoordinateSystemTypeEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId CoordinateSystemTypeEnum = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.CoordinateSystemTypeEnum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the IDDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId IDDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.IDDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CodeDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId CodeDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.CodeDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ComponentIdDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId ComponentIdDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.ComponentIdDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ConnectionCodeMachineSideDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionCodeMachineSideDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.ConnectionCodeMachineSideDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CountValueDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId CountValueDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.CountValueDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CreationTimeDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId CreationTimeDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.CreationTimeDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CutterStatusValueTypeEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId CutterStatusValueTypeEnum = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.CutterStatusValueTypeEnum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DataItemEnumDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId DataItemEnumDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.DataItemEnumDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DataItemEnumTypeEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId DataItemEnumTypeEnum = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.DataItemEnumTypeEnum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DataItemIdDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId DataItemIdDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.DataItemIdDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DataItemStatisticsDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId DataItemStatisticsDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.DataItemStatisticsDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DataItemStatisticsTypeEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId DataItemStatisticsTypeEnum = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.DataItemStatisticsTypeEnum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DataItemSubEnumTypeEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId DataItemSubEnumTypeEnum = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.DataItemSubEnumTypeEnum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DefinitionFormatTypeEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId DefinitionFormatTypeEnum = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.DefinitionFormatTypeEnum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DecibelValueDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId DecibelValueDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.DecibelValueDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DescriptionTextDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId DescriptionTextDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.DescriptionTextDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DirectionTypeEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId DirectionTypeEnum = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.DirectionTypeEnum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DoorStateTypeEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId DoorStateTypeEnum = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.DoorStateTypeEnum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DurationTimeDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId DurationTimeDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.DurationTimeDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DurationValueDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId DurationValueDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.DurationValueDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the EdgeCountDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId EdgeCountDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.EdgeCountDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the EmergencyStopTypeEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId EmergencyStopTypeEnum = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.EmergencyStopTypeEnum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the EnergyValueDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId EnergyValueDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.EnergyValueDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ExecutionTypeEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId ExecutionTypeEnum = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.ExecutionTypeEnum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the FeedrateValueDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId FeedrateValueDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.FeedrateValueDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the FirmwareVersionDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId FirmwareVersionDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.FirmwareVersionDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ForceValueDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId ForceValueDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.ForceValueDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the GradeDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId GradeDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.GradeDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the IndexRangeDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId IndexRangeDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.IndexRangeDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the InstanceIdDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId InstanceIdDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.InstanceIdDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ItemIdDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId ItemIdDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.ItemIdDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the InterfaceStateTypeEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId InterfaceStateTypeEnum = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.InterfaceStateTypeEnum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ItemSourceDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId ItemSourceDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.ItemSourceDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the LocationSizeDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId LocationSizeDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.LocationSizeDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the LocationValueDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId LocationValueDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.LocationValueDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the LocationsTypeEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId LocationsTypeEnum = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.LocationsTypeEnum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the LocusDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId LocusDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.LocusDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ManufacturersDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId ManufacturersDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.ManufacturersDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MassValueDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId MassValueDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.MassValueDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MaximumCountDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId MaximumCountDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.MaximumCountDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MaximumDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId MaximumDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.MaximumDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MeasurementValueDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId MeasurementValueDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.MeasurementValueDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MinimumDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId MinimumDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.MinimumDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ModelDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId ModelDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.ModelDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the NameDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId NameDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.NameDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the NativeCodeDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId NativeCodeDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.NativeCodeDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the NativeNotifcationCodeDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId NativeNotifcationCodeDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.NativeNotifcationCodeDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the NativeScaleDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId NativeScaleDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.NativeScaleDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the NativeSeverityDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId NativeSeverityDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.NativeSeverityDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the NextCalibrationDateDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId NextCalibrationDateDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.NextCalibrationDateDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the NominalDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId NominalDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.NominalDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the NotificationCodeTypeEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId NotificationCodeTypeEnum = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.NotificationCodeTypeEnum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the NotificationDescriptionDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId NotificationDescriptionDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.NotificationDescriptionDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OccurrenceTimeDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId OccurrenceTimeDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.OccurrenceTimeDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OverlapDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId OverlapDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.OverlapDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PathModeTypeEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId PathModeTypeEnum = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.PathModeTypeEnum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PowerStateTypeEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId PowerStateTypeEnum = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.PowerStateTypeEnum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ProgramToolNumberDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId ProgramToolNumberDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.ProgramToolNumberDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the QualifierTypeEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId QualifierTypeEnum = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.QualifierTypeEnum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the RateDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId RateDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.RateDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ReconditionCountValueDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId ReconditionCountValueDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.ReconditionCountValueDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the RotaryModeTypeEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId RotaryModeTypeEnum = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.RotaryModeTypeEnum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the SampleRateDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId SampleRateDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.SampleRateDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the SeverityTypeEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId SeverityTypeEnum = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.SeverityTypeEnum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the SignificantDigitsValueDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId SignificantDigitsValueDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.SignificantDigitsValueDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the StationDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId StationDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.StationDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the SenderDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId SenderDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.SenderDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the SequenceDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId SequenceDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.SequenceDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the SerialNumberDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId SerialNumberDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.SerialNumberDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the SpeedDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId SpeedDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.SpeedDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TestIndicatorDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId TestIndicatorDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.TestIndicatorDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ToolEventValueDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId ToolEventValueDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.ToolEventValueDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ToolGroupDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId ToolGroupDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.ToolGroupDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ToolIdDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId ToolIdDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.ToolIdDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ToolLifeDirectionTypeEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId ToolLifeDirectionTypeEnum = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.ToolLifeDirectionTypeEnum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ToolLifeTypeEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId ToolLifeTypeEnum = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.ToolLifeTypeEnum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ToolLifeValueDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId ToolLifeValueDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.ToolLifeValueDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the UnitsExtDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId UnitsExtDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.UnitsExtDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VersionDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId VersionDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.VersionDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VibrationValueDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId VibrationValueDataType = new ExpandedNodeId(Opc.Ua.MTConnect.DataTypes.VibrationValueDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);
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
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_Open Method.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnectNamespaceMetadata_NamespaceFile_Open = new ExpandedNodeId(Opc.Ua.MTConnect.Methods.OpcUaMTConnectNamespaceMetadata_NamespaceFile_Open, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_Close Method.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnectNamespaceMetadata_NamespaceFile_Close = new ExpandedNodeId(Opc.Ua.MTConnect.Methods.OpcUaMTConnectNamespaceMetadata_NamespaceFile_Close, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_Read Method.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnectNamespaceMetadata_NamespaceFile_Read = new ExpandedNodeId(Opc.Ua.MTConnect.Methods.OpcUaMTConnectNamespaceMetadata_NamespaceFile_Read, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_Write Method.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnectNamespaceMetadata_NamespaceFile_Write = new ExpandedNodeId(Opc.Ua.MTConnect.Methods.OpcUaMTConnectNamespaceMetadata_NamespaceFile_Write, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_GetPosition Method.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnectNamespaceMetadata_NamespaceFile_GetPosition = new ExpandedNodeId(Opc.Ua.MTConnect.Methods.OpcUaMTConnectNamespaceMetadata_NamespaceFile_GetPosition, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_SetPosition Method.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnectNamespaceMetadata_NamespaceFile_SetPosition = new ExpandedNodeId(Opc.Ua.MTConnect.Methods.OpcUaMTConnectNamespaceMetadata_NamespaceFile_SetPosition, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);
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
        /// The identifier for the OpcUaMTConnectNamespaceMetadata Object.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnectNamespaceMetadata = new ExpandedNodeId(Opc.Ua.MTConnect.Objects.OpcUaMTConnectNamespaceMetadata, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTDevices Object.
        /// </summary>
        public static readonly ExpandedNodeId MTDevices = new ExpandedNodeId(Opc.Ua.MTConnect.Objects.MTDevices, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTDeviceType_DataItems Object.
        /// </summary>
        public static readonly ExpandedNodeId MTDeviceType_DataItems = new ExpandedNodeId(Opc.Ua.MTConnect.Objects.MTDeviceType_DataItems, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTDeviceType_Components Object.
        /// </summary>
        public static readonly ExpandedNodeId MTDeviceType_Components = new ExpandedNodeId(Opc.Ua.MTConnect.Objects.MTDeviceType_Components, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTDeviceType_Conditions Object.
        /// </summary>
        public static readonly ExpandedNodeId MTDeviceType_Conditions = new ExpandedNodeId(Opc.Ua.MTConnect.Objects.MTDeviceType_Conditions, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTConditionType_LimitState Object.
        /// </summary>
        public static readonly ExpandedNodeId MTConditionType_LimitState = new ExpandedNodeId(Opc.Ua.MTConnect.Objects.MTConditionType_LimitState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CuttingItemType_ItemLife Object.
        /// </summary>
        public static readonly ExpandedNodeId CuttingItemType_ItemLife = new ExpandedNodeId(Opc.Ua.MTConnect.Objects.CuttingItemType_ItemLife, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CuttingItemType_Measurements Object.
        /// </summary>
        public static readonly ExpandedNodeId CuttingItemType_Measurements = new ExpandedNodeId(Opc.Ua.MTConnect.Objects.CuttingItemType_Measurements, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DataItemEnumDataType_Encoding_DefaultBinary Object.
        /// </summary>
        public static readonly ExpandedNodeId DataItemEnumDataType_Encoding_DefaultBinary = new ExpandedNodeId(Opc.Ua.MTConnect.Objects.DataItemEnumDataType_Encoding_DefaultBinary, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DataItemStatisticsDataType_Encoding_DefaultBinary Object.
        /// </summary>
        public static readonly ExpandedNodeId DataItemStatisticsDataType_Encoding_DefaultBinary = new ExpandedNodeId(Opc.Ua.MTConnect.Objects.DataItemStatisticsDataType_Encoding_DefaultBinary, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DataItemEnumDataType_Encoding_DefaultXml Object.
        /// </summary>
        public static readonly ExpandedNodeId DataItemEnumDataType_Encoding_DefaultXml = new ExpandedNodeId(Opc.Ua.MTConnect.Objects.DataItemEnumDataType_Encoding_DefaultXml, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DataItemStatisticsDataType_Encoding_DefaultXml Object.
        /// </summary>
        public static readonly ExpandedNodeId DataItemStatisticsDataType_Encoding_DefaultXml = new ExpandedNodeId(Opc.Ua.MTConnect.Objects.DataItemStatisticsDataType_Encoding_DefaultXml, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DataItemEnumDataType_Encoding_DefaultJson Object.
        /// </summary>
        public static readonly ExpandedNodeId DataItemEnumDataType_Encoding_DefaultJson = new ExpandedNodeId(Opc.Ua.MTConnect.Objects.DataItemEnumDataType_Encoding_DefaultJson, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DataItemStatisticsDataType_Encoding_DefaultJson Object.
        /// </summary>
        public static readonly ExpandedNodeId DataItemStatisticsDataType_Encoding_DefaultJson = new ExpandedNodeId(Opc.Ua.MTConnect.Objects.DataItemStatisticsDataType_Encoding_DefaultJson, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);
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
        /// The identifier for the MTDevicesType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId MTDevicesType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.MTDevicesType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTDeviceType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId MTDeviceType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.MTDeviceType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTComponentType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId MTComponentType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.MTComponentType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTAxesType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId MTAxesType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.MTAxesType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTControllerType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId MTControllerType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.MTControllerType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTDoorType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId MTDoorType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.MTDoorType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTActuatorType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId MTActuatorType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.MTActuatorType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId MTConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.MTConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AccelerationConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId AccelerationConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.AccelerationConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Accumulated_TimeConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId Accumulated_TimeConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.Accumulated_TimeConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AmperageConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId AmperageConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.AmperageConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AngleConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId AngleConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.AngleConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Angular-AccelerationConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId Angular-AccelerationConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.Angular-AccelerationConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Angular_VelocityConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId Angular_VelocityConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.Angular_VelocityConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CommunicationsConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationsConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.CommunicationsConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ConcentrationConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ConcentrationConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.ConcentrationConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ConductivityConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ConductivityConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.ConductivityConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Data_RangeConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId Data_RangeConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.Data_RangeConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DirectionConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId DirectionConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.DirectionConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DisplacementConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId DisplacementConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.DisplacementConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Electrical_EnergyConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId Electrical_EnergyConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.Electrical_EnergyConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Fill_LevelConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId Fill_LevelConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.Fill_LevelConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the FlowConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId FlowConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.FlowConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the FrequencyConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId FrequencyConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.FrequencyConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the HardwareConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId HardwareConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.HardwareConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Linear_ForceConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId Linear_ForceConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.Linear_ForceConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the LoadConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId LoadConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.LoadConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Logic_ProgramConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId Logic_ProgramConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.Logic_ProgramConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MassConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId MassConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.MassConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Motion_ProgramConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId Motion_ProgramConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.Motion_ProgramConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Path_FeedrateConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId Path_FeedrateConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.Path_FeedrateConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Path_PositionConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId Path_PositionConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.Path_PositionConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PHConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId PHConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.PHConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PositionConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId PositionConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.PositionConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Power_FactorConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId Power_FactorConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.Power_FactorConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PressureConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId PressureConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.PressureConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ResistanceConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ResistanceConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.ResistanceConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Rotary_VelocityConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId Rotary_VelocityConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.Rotary_VelocityConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Sound_LevelConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId Sound_LevelConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.Sound_LevelConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the StrainConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId StrainConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.StrainConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the SystemConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId SystemConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.SystemConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TemperatureConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId TemperatureConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.TemperatureConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TiltConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId TiltConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.TiltConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TorqueConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId TorqueConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.TorqueConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VelocityConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId VelocityConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.VelocityConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ViscosityConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ViscosityConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.ViscosityConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VoltageConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId VoltageConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.VoltageConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Volt_AmperageConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId Volt_AmperageConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.Volt_AmperageConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VoltAmperageReactiveConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId VoltAmperageReactiveConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.VoltAmperageReactiveConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the WattageConditionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId WattageConditionType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.WattageConditionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ConfigurationType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ConfigurationType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.ConfigurationType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the SensorConfigurationType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId SensorConfigurationType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.SensorConfigurationType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the SensorType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId SensorType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.SensorType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the SourceType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId SourceType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.SourceType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AssetType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId AssetType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.AssetType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CuttingToolType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId CuttingToolType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.CuttingToolType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId CuttingToolLifeCycleType = new ExpandedNodeId(Opc.Ua.MTConnect.ObjectTypes.CuttingToolLifeCycleType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);
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
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnectNamespaceMetadata_NamespaceUri = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.OpcUaMTConnectNamespaceMetadata_NamespaceUri, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceVersion Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnectNamespaceMetadata_NamespaceVersion = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.OpcUaMTConnectNamespaceMetadata_NamespaceVersion, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespacePublicationDate Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnectNamespaceMetadata_NamespacePublicationDate = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.OpcUaMTConnectNamespaceMetadata_NamespacePublicationDate, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_IsNamespaceSubset Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnectNamespaceMetadata_IsNamespaceSubset = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.OpcUaMTConnectNamespaceMetadata_IsNamespaceSubset, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_StaticNodeIdTypes Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnectNamespaceMetadata_StaticNodeIdTypes = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.OpcUaMTConnectNamespaceMetadata_StaticNodeIdTypes, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_StaticNumericNodeIdRange Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnectNamespaceMetadata_StaticNumericNodeIdRange = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.OpcUaMTConnectNamespaceMetadata_StaticNumericNodeIdRange, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_StaticStringNodeIdPattern Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnectNamespaceMetadata_StaticStringNodeIdPattern = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.OpcUaMTConnectNamespaceMetadata_StaticStringNodeIdPattern, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_Size Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnectNamespaceMetadata_NamespaceFile_Size = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.OpcUaMTConnectNamespaceMetadata_NamespaceFile_Size, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_Writable Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnectNamespaceMetadata_NamespaceFile_Writable = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.OpcUaMTConnectNamespaceMetadata_NamespaceFile_Writable, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_UserWritable Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnectNamespaceMetadata_NamespaceFile_UserWritable = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.OpcUaMTConnectNamespaceMetadata_NamespaceFile_UserWritable, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_OpenCount Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnectNamespaceMetadata_NamespaceFile_OpenCount = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.OpcUaMTConnectNamespaceMetadata_NamespaceFile_OpenCount, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_Open_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnectNamespaceMetadata_NamespaceFile_Open_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.OpcUaMTConnectNamespaceMetadata_NamespaceFile_Open_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_Open_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnectNamespaceMetadata_NamespaceFile_Open_OutputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.OpcUaMTConnectNamespaceMetadata_NamespaceFile_Open_OutputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_Close_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnectNamespaceMetadata_NamespaceFile_Close_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.OpcUaMTConnectNamespaceMetadata_NamespaceFile_Close_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_Read_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnectNamespaceMetadata_NamespaceFile_Read_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.OpcUaMTConnectNamespaceMetadata_NamespaceFile_Read_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_Read_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnectNamespaceMetadata_NamespaceFile_Read_OutputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.OpcUaMTConnectNamespaceMetadata_NamespaceFile_Read_OutputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_Write_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnectNamespaceMetadata_NamespaceFile_Write_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.OpcUaMTConnectNamespaceMetadata_NamespaceFile_Write_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_GetPosition_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnectNamespaceMetadata_NamespaceFile_GetPosition_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.OpcUaMTConnectNamespaceMetadata_NamespaceFile_GetPosition_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_GetPosition_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnectNamespaceMetadata_NamespaceFile_GetPosition_OutputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.OpcUaMTConnectNamespaceMetadata_NamespaceFile_GetPosition_OutputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_NamespaceFile_SetPosition_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnectNamespaceMetadata_NamespaceFile_SetPosition_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.OpcUaMTConnectNamespaceMetadata_NamespaceFile_SetPosition_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_DefaultRolePermissions Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnectNamespaceMetadata_DefaultRolePermissions = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.OpcUaMTConnectNamespaceMetadata_DefaultRolePermissions, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_DefaultUserRolePermissions Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnectNamespaceMetadata_DefaultUserRolePermissions = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.OpcUaMTConnectNamespaceMetadata_DefaultUserRolePermissions, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnectNamespaceMetadata_DefaultAccessRestrictions Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnectNamespaceMetadata_DefaultAccessRestrictions = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.OpcUaMTConnectNamespaceMetadata_DefaultAccessRestrictions, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTDevicesType_MTConnectVersion Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTDevicesType_MTConnectVersion = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTDevicesType_MTConnectVersion, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTDevicesType_OPCUAMappingDate Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTDevicesType_OPCUAMappingDate = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTDevicesType_OPCUAMappingDate, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTDevicesType_OPCUAVersion Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTDevicesType_OPCUAVersion = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTDevicesType_OPCUAVersion, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTDeviceType_Availability Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTDeviceType_Availability = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTDeviceType_Availability, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTDeviceType_Manufacturer Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTDeviceType_Manufacturer = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTDeviceType_Manufacturer, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTDeviceType_SerialNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTDeviceType_SerialNumber = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTDeviceType_SerialNumber, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTDeviceType_Configuration Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTDeviceType_Configuration = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTDeviceType_Configuration, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTDeviceType_SampleInterval Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTDeviceType_SampleInterval = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTDeviceType_SampleInterval, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTComponentType_Model Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTComponentType_Model = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTComponentType_Model, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTComponentType_Station Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTComponentType_Station = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTComponentType_Station, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTAxesType_LinearAxesX Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTAxesType_LinearAxesX = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTAxesType_LinearAxesX, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTAxesType_LinearAxesY Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTAxesType_LinearAxesY = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTAxesType_LinearAxesY, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTAxesType_LinearAxesZ Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTAxesType_LinearAxesZ = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTAxesType_LinearAxesZ, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTAxesType_RotoryAxesA Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTAxesType_RotoryAxesA = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTAxesType_RotoryAxesA, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTAxesType_RotoryAxesB Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTAxesType_RotoryAxesB = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTAxesType_RotoryAxesB, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTAxesType_RotoryAxesC Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTAxesType_RotoryAxesC = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTAxesType_RotoryAxesC, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTDoorType_DoorState Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTDoorType_DoorState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTDoorType_DoorState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTActuatorType_LinearAxesX Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTActuatorType_LinearAxesX = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTActuatorType_LinearAxesX, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTSampleDataItemType_CoordinateSystem Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTSampleDataItemType_CoordinateSystem = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTSampleDataItemType_CoordinateSystem, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTSampleDataItemType_NativeUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTSampleDataItemType_NativeUnits = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTSampleDataItemType_NativeUnits, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTSampleDataItemType_NativeScale Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTSampleDataItemType_NativeScale = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTSampleDataItemType_NativeScale, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTSampleDataItemType_SampleInterval Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTSampleDataItemType_SampleInterval = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTSampleDataItemType_SampleInterval, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTEventDataItemType_CoordinateSystem Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTEventDataItemType_CoordinateSystem = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTEventDataItemType_CoordinateSystem, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTEventDataItemType_SampleInterval Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTEventDataItemType_SampleInterval = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTEventDataItemType_SampleInterval, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTConditionType_MTCurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTConditionType_MTCurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTConditionType_MTCurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTConditionType_ActiveState Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTConditionType_ActiveState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTConditionType_ActiveState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTConditionType_NativeCode Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTConditionType_NativeCode = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTConditionType_NativeCode, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTConditionType_NativeSeverity Variable.
        /// </summary>
        public static readonly ExpandedNodeId MTConditionType_NativeSeverity = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MTConditionType_NativeSeverity, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AccelerationConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId AccelerationConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AccelerationConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AccelerationConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId AccelerationConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AccelerationConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AccelerationConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId AccelerationConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AccelerationConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AccelerationConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId AccelerationConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AccelerationConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AccelerationConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AccelerationConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AccelerationConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AccelerationConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AccelerationConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AccelerationConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AccelerationConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AccelerationConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AccelerationConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AccelerationConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId AccelerationConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AccelerationConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AccelerationConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId AccelerationConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AccelerationConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AccelerationConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId AccelerationConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AccelerationConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AccelerationConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId AccelerationConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AccelerationConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Accumulated_TimeConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Accumulated_TimeConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Accumulated_TimeConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Accumulated_TimeConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Accumulated_TimeConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Accumulated_TimeConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Accumulated_TimeConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Accumulated_TimeConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Accumulated_TimeConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Accumulated_TimeConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Accumulated_TimeConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Accumulated_TimeConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Accumulated_TimeConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Accumulated_TimeConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Accumulated_TimeConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Accumulated_TimeConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Accumulated_TimeConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Accumulated_TimeConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Accumulated_TimeConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Accumulated_TimeConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Accumulated_TimeConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Accumulated_TimeConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Accumulated_TimeConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Accumulated_TimeConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Accumulated_TimeConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId Accumulated_TimeConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Accumulated_TimeConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Accumulated_TimeConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Accumulated_TimeConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Accumulated_TimeConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Accumulated_TimeConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Accumulated_TimeConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Accumulated_TimeConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AmperageConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId AmperageConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AmperageConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AmperageConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId AmperageConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AmperageConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AmperageConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId AmperageConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AmperageConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AmperageConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId AmperageConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AmperageConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AmperageConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AmperageConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AmperageConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AmperageConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AmperageConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AmperageConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AmperageConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AmperageConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AmperageConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AmperageConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId AmperageConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AmperageConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AmperageConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId AmperageConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AmperageConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AmperageConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId AmperageConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AmperageConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AmperageConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId AmperageConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AmperageConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AngleConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId AngleConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AngleConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AngleConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId AngleConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AngleConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AngleConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId AngleConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AngleConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AngleConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId AngleConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AngleConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AngleConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AngleConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AngleConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AngleConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AngleConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AngleConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AngleConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AngleConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AngleConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AngleConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId AngleConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AngleConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AngleConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId AngleConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AngleConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AngleConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId AngleConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AngleConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AngleConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId AngleConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AngleConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Angular-AccelerationConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Angular-AccelerationConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Angular-AccelerationConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Angular-AccelerationConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Angular-AccelerationConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Angular-AccelerationConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Angular-AccelerationConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Angular-AccelerationConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Angular-AccelerationConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Angular-AccelerationConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Angular-AccelerationConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Angular-AccelerationConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Angular-AccelerationConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Angular-AccelerationConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Angular-AccelerationConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Angular-AccelerationConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Angular-AccelerationConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Angular-AccelerationConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Angular-AccelerationConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Angular-AccelerationConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Angular-AccelerationConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Angular-AccelerationConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Angular-AccelerationConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Angular-AccelerationConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Angular-AccelerationConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId Angular-AccelerationConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Angular-AccelerationConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Angular-AccelerationConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Angular-AccelerationConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Angular-AccelerationConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Angular-AccelerationConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Angular-AccelerationConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Angular-AccelerationConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Angular_VelocityConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Angular_VelocityConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Angular_VelocityConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Angular_VelocityConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Angular_VelocityConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Angular_VelocityConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Angular_VelocityConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Angular_VelocityConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Angular_VelocityConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Angular_VelocityConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Angular_VelocityConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Angular_VelocityConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Angular_VelocityConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Angular_VelocityConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Angular_VelocityConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Angular_VelocityConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Angular_VelocityConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Angular_VelocityConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Angular_VelocityConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Angular_VelocityConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Angular_VelocityConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Angular_VelocityConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Angular_VelocityConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Angular_VelocityConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Angular_VelocityConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId Angular_VelocityConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Angular_VelocityConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Angular_VelocityConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Angular_VelocityConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Angular_VelocityConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Angular_VelocityConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Angular_VelocityConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Angular_VelocityConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CommunicationsConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationsConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CommunicationsConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CommunicationsConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationsConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CommunicationsConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CommunicationsConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationsConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CommunicationsConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CommunicationsConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationsConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CommunicationsConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CommunicationsConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationsConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CommunicationsConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CommunicationsConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationsConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CommunicationsConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CommunicationsConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationsConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CommunicationsConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CommunicationsConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationsConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CommunicationsConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CommunicationsConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationsConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CommunicationsConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CommunicationsConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationsConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CommunicationsConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CommunicationsConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationsConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CommunicationsConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ConcentrationConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConcentrationConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ConcentrationConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ConcentrationConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConcentrationConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ConcentrationConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ConcentrationConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConcentrationConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ConcentrationConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ConcentrationConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConcentrationConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ConcentrationConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ConcentrationConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConcentrationConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ConcentrationConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ConcentrationConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConcentrationConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ConcentrationConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ConcentrationConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConcentrationConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ConcentrationConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ConcentrationConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConcentrationConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ConcentrationConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ConcentrationConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConcentrationConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ConcentrationConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ConcentrationConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConcentrationConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ConcentrationConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ConcentrationConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConcentrationConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ConcentrationConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ConductivityConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConductivityConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ConductivityConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ConductivityConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConductivityConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ConductivityConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ConductivityConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConductivityConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ConductivityConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ConductivityConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConductivityConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ConductivityConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ConductivityConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConductivityConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ConductivityConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ConductivityConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConductivityConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ConductivityConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ConductivityConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConductivityConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ConductivityConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ConductivityConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConductivityConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ConductivityConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ConductivityConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConductivityConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ConductivityConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ConductivityConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConductivityConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ConductivityConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ConductivityConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConductivityConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ConductivityConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Data_RangeConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Data_RangeConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Data_RangeConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Data_RangeConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Data_RangeConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Data_RangeConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Data_RangeConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Data_RangeConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Data_RangeConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Data_RangeConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Data_RangeConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Data_RangeConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Data_RangeConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Data_RangeConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Data_RangeConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Data_RangeConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Data_RangeConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Data_RangeConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Data_RangeConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Data_RangeConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Data_RangeConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Data_RangeConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Data_RangeConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Data_RangeConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Data_RangeConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId Data_RangeConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Data_RangeConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Data_RangeConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Data_RangeConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Data_RangeConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Data_RangeConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Data_RangeConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Data_RangeConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DirectionConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectionConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.DirectionConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DirectionConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectionConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.DirectionConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DirectionConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectionConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.DirectionConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DirectionConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectionConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.DirectionConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DirectionConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectionConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.DirectionConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DirectionConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectionConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.DirectionConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DirectionConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectionConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.DirectionConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DirectionConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectionConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.DirectionConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DirectionConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectionConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.DirectionConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DirectionConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectionConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.DirectionConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DirectionConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectionConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.DirectionConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DisplacementConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId DisplacementConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.DisplacementConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DisplacementConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId DisplacementConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.DisplacementConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DisplacementConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId DisplacementConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.DisplacementConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DisplacementConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId DisplacementConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.DisplacementConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DisplacementConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DisplacementConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.DisplacementConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DisplacementConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DisplacementConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.DisplacementConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DisplacementConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DisplacementConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.DisplacementConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DisplacementConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId DisplacementConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.DisplacementConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DisplacementConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId DisplacementConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.DisplacementConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DisplacementConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId DisplacementConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.DisplacementConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DisplacementConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId DisplacementConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.DisplacementConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Electrical_EnergyConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Electrical_EnergyConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Electrical_EnergyConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Electrical_EnergyConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Electrical_EnergyConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Electrical_EnergyConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Electrical_EnergyConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Electrical_EnergyConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Electrical_EnergyConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Electrical_EnergyConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Electrical_EnergyConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Electrical_EnergyConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Electrical_EnergyConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Electrical_EnergyConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Electrical_EnergyConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Electrical_EnergyConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Electrical_EnergyConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Electrical_EnergyConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Electrical_EnergyConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Electrical_EnergyConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Electrical_EnergyConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Electrical_EnergyConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Electrical_EnergyConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Electrical_EnergyConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Electrical_EnergyConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId Electrical_EnergyConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Electrical_EnergyConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Electrical_EnergyConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Electrical_EnergyConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Electrical_EnergyConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Electrical_EnergyConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Electrical_EnergyConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Electrical_EnergyConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Fill_LevelConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Fill_LevelConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Fill_LevelConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Fill_LevelConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Fill_LevelConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Fill_LevelConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Fill_LevelConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Fill_LevelConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Fill_LevelConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Fill_LevelConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Fill_LevelConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Fill_LevelConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Fill_LevelConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Fill_LevelConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Fill_LevelConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Fill_LevelConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Fill_LevelConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Fill_LevelConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Fill_LevelConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Fill_LevelConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Fill_LevelConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Fill_LevelConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Fill_LevelConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Fill_LevelConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Fill_LevelConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId Fill_LevelConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Fill_LevelConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Fill_LevelConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Fill_LevelConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Fill_LevelConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Fill_LevelConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Fill_LevelConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Fill_LevelConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the FlowConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId FlowConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.FlowConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the FlowConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId FlowConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.FlowConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the FlowConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId FlowConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.FlowConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the FlowConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId FlowConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.FlowConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the FlowConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId FlowConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.FlowConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the FlowConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId FlowConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.FlowConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the FlowConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId FlowConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.FlowConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the FlowConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId FlowConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.FlowConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the FlowConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId FlowConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.FlowConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the FlowConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId FlowConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.FlowConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the FlowConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId FlowConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.FlowConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the FrequencyConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId FrequencyConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.FrequencyConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the FrequencyConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId FrequencyConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.FrequencyConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the FrequencyConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId FrequencyConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.FrequencyConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the FrequencyConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId FrequencyConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.FrequencyConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the FrequencyConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId FrequencyConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.FrequencyConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the FrequencyConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId FrequencyConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.FrequencyConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the FrequencyConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId FrequencyConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.FrequencyConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the FrequencyConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId FrequencyConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.FrequencyConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the FrequencyConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId FrequencyConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.FrequencyConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the FrequencyConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId FrequencyConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.FrequencyConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the FrequencyConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId FrequencyConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.FrequencyConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the HardwareConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId HardwareConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.HardwareConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the HardwareConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId HardwareConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.HardwareConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the HardwareConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId HardwareConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.HardwareConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the HardwareConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId HardwareConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.HardwareConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the HardwareConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId HardwareConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.HardwareConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the HardwareConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId HardwareConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.HardwareConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the HardwareConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId HardwareConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.HardwareConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the HardwareConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId HardwareConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.HardwareConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the HardwareConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId HardwareConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.HardwareConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the HardwareConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId HardwareConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.HardwareConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the HardwareConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId HardwareConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.HardwareConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Linear_ForceConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Linear_ForceConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Linear_ForceConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Linear_ForceConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Linear_ForceConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Linear_ForceConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Linear_ForceConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Linear_ForceConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Linear_ForceConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Linear_ForceConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Linear_ForceConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Linear_ForceConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Linear_ForceConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Linear_ForceConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Linear_ForceConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Linear_ForceConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Linear_ForceConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Linear_ForceConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Linear_ForceConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Linear_ForceConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Linear_ForceConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Linear_ForceConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Linear_ForceConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Linear_ForceConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Linear_ForceConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId Linear_ForceConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Linear_ForceConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Linear_ForceConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Linear_ForceConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Linear_ForceConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Linear_ForceConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Linear_ForceConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Linear_ForceConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the LoadConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId LoadConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.LoadConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the LoadConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId LoadConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.LoadConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the LoadConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId LoadConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.LoadConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the LoadConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId LoadConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.LoadConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the LoadConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId LoadConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.LoadConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the LoadConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId LoadConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.LoadConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the LoadConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId LoadConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.LoadConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the LoadConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId LoadConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.LoadConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the LoadConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId LoadConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.LoadConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the LoadConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId LoadConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.LoadConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the LoadConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId LoadConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.LoadConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Logic_ProgramConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Logic_ProgramConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Logic_ProgramConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Logic_ProgramConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Logic_ProgramConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Logic_ProgramConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Logic_ProgramConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Logic_ProgramConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Logic_ProgramConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Logic_ProgramConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Logic_ProgramConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Logic_ProgramConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Logic_ProgramConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Logic_ProgramConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Logic_ProgramConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Logic_ProgramConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Logic_ProgramConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Logic_ProgramConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Logic_ProgramConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Logic_ProgramConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Logic_ProgramConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Logic_ProgramConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Logic_ProgramConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Logic_ProgramConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Logic_ProgramConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId Logic_ProgramConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Logic_ProgramConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Logic_ProgramConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Logic_ProgramConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Logic_ProgramConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Logic_ProgramConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Logic_ProgramConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Logic_ProgramConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MassConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId MassConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MassConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MassConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId MassConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MassConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MassConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId MassConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MassConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MassConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId MassConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MassConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MassConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MassConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MassConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MassConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MassConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MassConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MassConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MassConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MassConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MassConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId MassConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MassConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MassConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId MassConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MassConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MassConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId MassConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MassConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MassConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId MassConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MassConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Motion_ProgramConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Motion_ProgramConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Motion_ProgramConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Motion_ProgramConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Motion_ProgramConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Motion_ProgramConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Motion_ProgramConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Motion_ProgramConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Motion_ProgramConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Motion_ProgramConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Motion_ProgramConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Motion_ProgramConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Motion_ProgramConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Motion_ProgramConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Motion_ProgramConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Motion_ProgramConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Motion_ProgramConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Motion_ProgramConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Motion_ProgramConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Motion_ProgramConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Motion_ProgramConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Motion_ProgramConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Motion_ProgramConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Motion_ProgramConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Motion_ProgramConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId Motion_ProgramConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Motion_ProgramConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Motion_ProgramConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Motion_ProgramConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Motion_ProgramConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Motion_ProgramConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Motion_ProgramConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Motion_ProgramConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Path_FeedrateConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Path_FeedrateConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Path_FeedrateConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Path_FeedrateConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Path_FeedrateConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Path_FeedrateConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Path_FeedrateConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Path_FeedrateConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Path_FeedrateConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Path_FeedrateConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Path_FeedrateConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Path_FeedrateConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Path_FeedrateConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Path_FeedrateConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Path_FeedrateConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Path_FeedrateConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Path_FeedrateConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Path_FeedrateConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Path_FeedrateConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Path_FeedrateConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Path_FeedrateConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Path_FeedrateConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Path_FeedrateConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Path_FeedrateConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Path_FeedrateConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId Path_FeedrateConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Path_FeedrateConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Path_FeedrateConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Path_FeedrateConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Path_FeedrateConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Path_FeedrateConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Path_FeedrateConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Path_FeedrateConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Path_PositionConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Path_PositionConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Path_PositionConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Path_PositionConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Path_PositionConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Path_PositionConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Path_PositionConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Path_PositionConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Path_PositionConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Path_PositionConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Path_PositionConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Path_PositionConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Path_PositionConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Path_PositionConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Path_PositionConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Path_PositionConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Path_PositionConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Path_PositionConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Path_PositionConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Path_PositionConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Path_PositionConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Path_PositionConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Path_PositionConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Path_PositionConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Path_PositionConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId Path_PositionConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Path_PositionConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Path_PositionConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Path_PositionConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Path_PositionConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Path_PositionConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Path_PositionConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Path_PositionConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PHConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId PHConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PHConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PHConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId PHConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PHConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PHConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId PHConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PHConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PHConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId PHConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PHConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PHConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PHConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PHConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PHConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PHConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PHConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PHConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PHConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PHConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PHConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId PHConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PHConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PHConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId PHConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PHConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PHConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId PHConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PHConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PHConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId PHConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PHConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PositionConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId PositionConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PositionConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PositionConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId PositionConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PositionConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PositionConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId PositionConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PositionConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PositionConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId PositionConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PositionConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PositionConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PositionConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PositionConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PositionConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PositionConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PositionConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PositionConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PositionConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PositionConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PositionConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId PositionConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PositionConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PositionConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId PositionConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PositionConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PositionConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId PositionConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PositionConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PositionConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId PositionConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PositionConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Power_FactorConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Power_FactorConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Power_FactorConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Power_FactorConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Power_FactorConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Power_FactorConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Power_FactorConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Power_FactorConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Power_FactorConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Power_FactorConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Power_FactorConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Power_FactorConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Power_FactorConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Power_FactorConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Power_FactorConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Power_FactorConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Power_FactorConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Power_FactorConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Power_FactorConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Power_FactorConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Power_FactorConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Power_FactorConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Power_FactorConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Power_FactorConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Power_FactorConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId Power_FactorConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Power_FactorConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Power_FactorConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Power_FactorConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Power_FactorConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Power_FactorConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Power_FactorConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Power_FactorConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PressureConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId PressureConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PressureConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PressureConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId PressureConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PressureConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PressureConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId PressureConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PressureConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PressureConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId PressureConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PressureConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PressureConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PressureConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PressureConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PressureConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PressureConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PressureConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PressureConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PressureConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PressureConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PressureConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId PressureConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PressureConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PressureConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId PressureConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PressureConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PressureConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId PressureConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PressureConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PressureConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId PressureConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PressureConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ResistanceConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId ResistanceConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ResistanceConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ResistanceConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId ResistanceConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ResistanceConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ResistanceConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId ResistanceConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ResistanceConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ResistanceConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId ResistanceConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ResistanceConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ResistanceConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ResistanceConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ResistanceConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ResistanceConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ResistanceConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ResistanceConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ResistanceConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ResistanceConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ResistanceConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ResistanceConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId ResistanceConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ResistanceConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ResistanceConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId ResistanceConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ResistanceConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ResistanceConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId ResistanceConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ResistanceConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ResistanceConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId ResistanceConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ResistanceConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Rotary_VelocityConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Rotary_VelocityConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Rotary_VelocityConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Rotary_VelocityConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Rotary_VelocityConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Rotary_VelocityConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Rotary_VelocityConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Rotary_VelocityConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Rotary_VelocityConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Rotary_VelocityConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Rotary_VelocityConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Rotary_VelocityConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Rotary_VelocityConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Rotary_VelocityConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Rotary_VelocityConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Rotary_VelocityConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Rotary_VelocityConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Rotary_VelocityConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Rotary_VelocityConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Rotary_VelocityConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Rotary_VelocityConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Rotary_VelocityConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Rotary_VelocityConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Rotary_VelocityConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Rotary_VelocityConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId Rotary_VelocityConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Rotary_VelocityConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Rotary_VelocityConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Rotary_VelocityConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Rotary_VelocityConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Rotary_VelocityConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Rotary_VelocityConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Rotary_VelocityConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Sound_LevelConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Sound_LevelConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Sound_LevelConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Sound_LevelConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Sound_LevelConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Sound_LevelConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Sound_LevelConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Sound_LevelConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Sound_LevelConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Sound_LevelConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Sound_LevelConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Sound_LevelConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Sound_LevelConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Sound_LevelConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Sound_LevelConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Sound_LevelConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Sound_LevelConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Sound_LevelConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Sound_LevelConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Sound_LevelConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Sound_LevelConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Sound_LevelConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Sound_LevelConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Sound_LevelConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Sound_LevelConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId Sound_LevelConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Sound_LevelConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Sound_LevelConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Sound_LevelConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Sound_LevelConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Sound_LevelConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Sound_LevelConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Sound_LevelConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the StrainConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId StrainConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.StrainConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the StrainConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId StrainConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.StrainConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the StrainConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId StrainConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.StrainConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the StrainConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId StrainConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.StrainConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the StrainConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId StrainConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.StrainConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the StrainConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId StrainConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.StrainConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the StrainConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId StrainConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.StrainConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the StrainConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId StrainConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.StrainConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the StrainConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId StrainConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.StrainConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the StrainConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId StrainConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.StrainConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the StrainConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId StrainConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.StrainConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the SystemConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId SystemConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.SystemConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the SystemConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId SystemConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.SystemConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the SystemConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId SystemConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.SystemConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the SystemConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId SystemConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.SystemConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the SystemConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SystemConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.SystemConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the SystemConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SystemConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.SystemConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the SystemConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SystemConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.SystemConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the SystemConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId SystemConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.SystemConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the SystemConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId SystemConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.SystemConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the SystemConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId SystemConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.SystemConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the SystemConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId SystemConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.SystemConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TemperatureConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId TemperatureConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.TemperatureConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TemperatureConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId TemperatureConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.TemperatureConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TemperatureConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId TemperatureConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.TemperatureConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TemperatureConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId TemperatureConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.TemperatureConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TemperatureConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TemperatureConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.TemperatureConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TemperatureConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TemperatureConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.TemperatureConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TemperatureConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TemperatureConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.TemperatureConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TemperatureConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId TemperatureConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.TemperatureConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TemperatureConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId TemperatureConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.TemperatureConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TemperatureConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId TemperatureConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.TemperatureConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TemperatureConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId TemperatureConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.TemperatureConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TiltConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId TiltConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.TiltConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TiltConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId TiltConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.TiltConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TiltConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId TiltConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.TiltConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TiltConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId TiltConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.TiltConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TiltConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TiltConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.TiltConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TiltConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TiltConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.TiltConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TiltConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TiltConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.TiltConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TiltConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId TiltConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.TiltConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TiltConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId TiltConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.TiltConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TiltConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId TiltConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.TiltConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TiltConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId TiltConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.TiltConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TorqueConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId TorqueConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.TorqueConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TorqueConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId TorqueConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.TorqueConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TorqueConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId TorqueConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.TorqueConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TorqueConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId TorqueConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.TorqueConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TorqueConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TorqueConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.TorqueConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TorqueConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TorqueConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.TorqueConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TorqueConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TorqueConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.TorqueConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TorqueConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId TorqueConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.TorqueConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TorqueConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId TorqueConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.TorqueConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TorqueConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId TorqueConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.TorqueConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TorqueConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId TorqueConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.TorqueConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VelocityConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId VelocityConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.VelocityConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VelocityConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId VelocityConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.VelocityConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VelocityConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId VelocityConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.VelocityConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VelocityConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId VelocityConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.VelocityConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VelocityConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId VelocityConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.VelocityConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VelocityConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId VelocityConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.VelocityConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VelocityConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId VelocityConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.VelocityConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VelocityConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId VelocityConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.VelocityConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VelocityConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId VelocityConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.VelocityConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VelocityConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId VelocityConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.VelocityConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VelocityConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId VelocityConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.VelocityConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ViscosityConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId ViscosityConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ViscosityConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ViscosityConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId ViscosityConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ViscosityConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ViscosityConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId ViscosityConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ViscosityConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ViscosityConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId ViscosityConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ViscosityConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ViscosityConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ViscosityConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ViscosityConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ViscosityConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ViscosityConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ViscosityConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ViscosityConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ViscosityConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ViscosityConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ViscosityConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId ViscosityConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ViscosityConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ViscosityConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId ViscosityConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ViscosityConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ViscosityConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId ViscosityConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ViscosityConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ViscosityConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId ViscosityConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ViscosityConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VoltageConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId VoltageConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.VoltageConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VoltageConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId VoltageConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.VoltageConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VoltageConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId VoltageConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.VoltageConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VoltageConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId VoltageConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.VoltageConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VoltageConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId VoltageConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.VoltageConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VoltageConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId VoltageConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.VoltageConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VoltageConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId VoltageConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.VoltageConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VoltageConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId VoltageConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.VoltageConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VoltageConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId VoltageConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.VoltageConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VoltageConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId VoltageConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.VoltageConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VoltageConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId VoltageConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.VoltageConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Volt_AmperageConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Volt_AmperageConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Volt_AmperageConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Volt_AmperageConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Volt_AmperageConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Volt_AmperageConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Volt_AmperageConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Volt_AmperageConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Volt_AmperageConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Volt_AmperageConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId Volt_AmperageConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Volt_AmperageConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Volt_AmperageConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Volt_AmperageConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Volt_AmperageConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Volt_AmperageConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Volt_AmperageConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Volt_AmperageConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Volt_AmperageConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Volt_AmperageConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Volt_AmperageConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Volt_AmperageConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Volt_AmperageConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Volt_AmperageConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Volt_AmperageConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId Volt_AmperageConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Volt_AmperageConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Volt_AmperageConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Volt_AmperageConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Volt_AmperageConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the Volt_AmperageConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId Volt_AmperageConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.Volt_AmperageConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VoltAmperageReactiveConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId VoltAmperageReactiveConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.VoltAmperageReactiveConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VoltAmperageReactiveConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId VoltAmperageReactiveConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.VoltAmperageReactiveConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VoltAmperageReactiveConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId VoltAmperageReactiveConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.VoltAmperageReactiveConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VoltAmperageReactiveConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId VoltAmperageReactiveConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.VoltAmperageReactiveConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VoltAmperageReactiveConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId VoltAmperageReactiveConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.VoltAmperageReactiveConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VoltAmperageReactiveConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId VoltAmperageReactiveConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.VoltAmperageReactiveConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VoltAmperageReactiveConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId VoltAmperageReactiveConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.VoltAmperageReactiveConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VoltAmperageReactiveConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId VoltAmperageReactiveConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.VoltAmperageReactiveConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VoltAmperageReactiveConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId VoltAmperageReactiveConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.VoltAmperageReactiveConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VoltAmperageReactiveConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId VoltAmperageReactiveConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.VoltAmperageReactiveConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VoltAmperageReactiveConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId VoltAmperageReactiveConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.VoltAmperageReactiveConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the WattageConditionType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId WattageConditionType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.WattageConditionType_EnabledState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the WattageConditionType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId WattageConditionType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.WattageConditionType_Quality_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the WattageConditionType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId WattageConditionType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.WattageConditionType_LastSeverity_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the WattageConditionType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId WattageConditionType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.WattageConditionType_Comment_SourceTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the WattageConditionType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId WattageConditionType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.WattageConditionType_AddComment_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the WattageConditionType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId WattageConditionType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.WattageConditionType_ConditionRefresh_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the WattageConditionType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId WattageConditionType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.WattageConditionType_ConditionRefresh2_InputArguments, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the WattageConditionType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId WattageConditionType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.WattageConditionType_ActiveState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the WattageConditionType_LimitState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId WattageConditionType_LimitState_CurrentState = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.WattageConditionType_LimitState_CurrentState, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the WattageConditionType_LimitState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId WattageConditionType_LimitState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.WattageConditionType_LimitState_CurrentState_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the WattageConditionType_LimitState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId WattageConditionType_LimitState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.WattageConditionType_LimitState_LastTransition_Id, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ChannelType_CalibrationDate Variable.
        /// </summary>
        public static readonly ExpandedNodeId ChannelType_CalibrationDate = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ChannelType_CalibrationDate, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ChannelType_NextCalibrationDate Variable.
        /// </summary>
        public static readonly ExpandedNodeId ChannelType_NextCalibrationDate = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ChannelType_NextCalibrationDate, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ChannelType_CalibrationInitials Variable.
        /// </summary>
        public static readonly ExpandedNodeId ChannelType_CalibrationInitials = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ChannelType_CalibrationInitials, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the SensorConfigurationType_FirmwareVersion Variable.
        /// </summary>
        public static readonly ExpandedNodeId SensorConfigurationType_FirmwareVersion = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.SensorConfigurationType_FirmwareVersion, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the SourceType_ComponentId Variable.
        /// </summary>
        public static readonly ExpandedNodeId SourceType_ComponentId = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.SourceType_ComponentId, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AssetType_SerialNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId AssetType_SerialNumber = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AssetType_SerialNumber, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AssetType_Manufacturers Variable.
        /// </summary>
        public static readonly ExpandedNodeId AssetType_Manufacturers = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AssetType_Manufacturers, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AssetType_LastChangeTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId AssetType_LastChangeTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AssetType_LastChangeTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AssetType_AssetDescription Variable.
        /// </summary>
        public static readonly ExpandedNodeId AssetType_AssetDescription = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AssetType_AssetDescription, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MeasurementType_SignificantDigits Variable.
        /// </summary>
        public static readonly ExpandedNodeId MeasurementType_SignificantDigits = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MeasurementType_SignificantDigits, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MeasurementType_Units Variable.
        /// </summary>
        public static readonly ExpandedNodeId MeasurementType_Units = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MeasurementType_Units, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MeasurementType_NativeUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId MeasurementType_NativeUnits = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MeasurementType_NativeUnits, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MeasurementType_LastChangeTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId MeasurementType_LastChangeTimestamp = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MeasurementType_LastChangeTimestamp, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MeasurementType_Code Variable.
        /// </summary>
        public static readonly ExpandedNodeId MeasurementType_Code = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MeasurementType_Code, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MeasurementType_Maximum Variable.
        /// </summary>
        public static readonly ExpandedNodeId MeasurementType_Maximum = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MeasurementType_Maximum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MeasurementType_Minimum Variable.
        /// </summary>
        public static readonly ExpandedNodeId MeasurementType_Minimum = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MeasurementType_Minimum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MeasurementType_Nominal Variable.
        /// </summary>
        public static readonly ExpandedNodeId MeasurementType_Nominal = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.MeasurementType_Nominal, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CuttingToolType_ToolId Variable.
        /// </summary>
        public static readonly ExpandedNodeId CuttingToolType_ToolId = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CuttingToolType_ToolId, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CuttingToolType_ToolGroup Variable.
        /// </summary>
        public static readonly ExpandedNodeId CuttingToolType_ToolGroup = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CuttingToolType_ToolGroup, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_CutterStatus Variable.
        /// </summary>
        public static readonly ExpandedNodeId CuttingToolLifeCycleType_CutterStatus = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CuttingToolLifeCycleType_CutterStatus, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_CutterStatus_Status Variable.
        /// </summary>
        public static readonly ExpandedNodeId CuttingToolLifeCycleType_CutterStatus_Status = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CuttingToolLifeCycleType_CutterStatus_Status, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_ReconditionCount Variable.
        /// </summary>
        public static readonly ExpandedNodeId CuttingToolLifeCycleType_ReconditionCount = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CuttingToolLifeCycleType_ReconditionCount, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_ToolLife Variable.
        /// </summary>
        public static readonly ExpandedNodeId CuttingToolLifeCycleType_ToolLife = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CuttingToolLifeCycleType_ToolLife, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_ToolLife_Type Variable.
        /// </summary>
        public static readonly ExpandedNodeId CuttingToolLifeCycleType_ToolLife_Type = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CuttingToolLifeCycleType_ToolLife_Type, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_ToolLife_CountDirection Variable.
        /// </summary>
        public static readonly ExpandedNodeId CuttingToolLifeCycleType_ToolLife_CountDirection = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CuttingToolLifeCycleType_ToolLife_CountDirection, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_ToolLife_Maximum Variable.
        /// </summary>
        public static readonly ExpandedNodeId CuttingToolLifeCycleType_ToolLife_Maximum = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CuttingToolLifeCycleType_ToolLife_Maximum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_ProgramToolNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId CuttingToolLifeCycleType_ProgramToolNumber = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CuttingToolLifeCycleType_ProgramToolNumber, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_ProgramToolNumber_Maximum Variable.
        /// </summary>
        public static readonly ExpandedNodeId CuttingToolLifeCycleType_ProgramToolNumber_Maximum = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CuttingToolLifeCycleType_ProgramToolNumber_Maximum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_ProgramToolNumber_Minimum Variable.
        /// </summary>
        public static readonly ExpandedNodeId CuttingToolLifeCycleType_ProgramToolNumber_Minimum = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CuttingToolLifeCycleType_ProgramToolNumber_Minimum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_ProgramToolNumber_Nominal Variable.
        /// </summary>
        public static readonly ExpandedNodeId CuttingToolLifeCycleType_ProgramToolNumber_Nominal = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CuttingToolLifeCycleType_ProgramToolNumber_Nominal, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_Location Variable.
        /// </summary>
        public static readonly ExpandedNodeId CuttingToolLifeCycleType_Location = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CuttingToolLifeCycleType_Location, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_Location_Type Variable.
        /// </summary>
        public static readonly ExpandedNodeId CuttingToolLifeCycleType_Location_Type = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CuttingToolLifeCycleType_Location_Type, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_Location_NegativeOverlap Variable.
        /// </summary>
        public static readonly ExpandedNodeId CuttingToolLifeCycleType_Location_NegativeOverlap = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CuttingToolLifeCycleType_Location_NegativeOverlap, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_Location_PositiveOverlap Variable.
        /// </summary>
        public static readonly ExpandedNodeId CuttingToolLifeCycleType_Location_PositiveOverlap = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CuttingToolLifeCycleType_Location_PositiveOverlap, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_ProgramSpindleSpeed Variable.
        /// </summary>
        public static readonly ExpandedNodeId CuttingToolLifeCycleType_ProgramSpindleSpeed = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CuttingToolLifeCycleType_ProgramSpindleSpeed, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_ProgramSpindleSpeed_Maximum Variable.
        /// </summary>
        public static readonly ExpandedNodeId CuttingToolLifeCycleType_ProgramSpindleSpeed_Maximum = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CuttingToolLifeCycleType_ProgramSpindleSpeed_Maximum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_ProgramSpindleSpeed_Minimum Variable.
        /// </summary>
        public static readonly ExpandedNodeId CuttingToolLifeCycleType_ProgramSpindleSpeed_Minimum = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CuttingToolLifeCycleType_ProgramSpindleSpeed_Minimum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_ProgramSpindleSpeed_Nominal Variable.
        /// </summary>
        public static readonly ExpandedNodeId CuttingToolLifeCycleType_ProgramSpindleSpeed_Nominal = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CuttingToolLifeCycleType_ProgramSpindleSpeed_Nominal, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_ProgramFeedRate Variable.
        /// </summary>
        public static readonly ExpandedNodeId CuttingToolLifeCycleType_ProgramFeedRate = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CuttingToolLifeCycleType_ProgramFeedRate, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CuttingToolLifeCycleType_ConnectionCodeMachineSide Variable.
        /// </summary>
        public static readonly ExpandedNodeId CuttingToolLifeCycleType_ConnectionCodeMachineSide = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CuttingToolLifeCycleType_ConnectionCodeMachineSide, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CutterStatusType_Status Variable.
        /// </summary>
        public static readonly ExpandedNodeId CutterStatusType_Status = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CutterStatusType_Status, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ReconditionCountType_MaximumCount Variable.
        /// </summary>
        public static readonly ExpandedNodeId ReconditionCountType_MaximumCount = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ReconditionCountType_MaximumCount, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the LifeType_Type Variable.
        /// </summary>
        public static readonly ExpandedNodeId LifeType_Type = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.LifeType_Type, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the LifeType_CountDirection Variable.
        /// </summary>
        public static readonly ExpandedNodeId LifeType_CountDirection = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.LifeType_CountDirection, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the LifeType_WarningLevel Variable.
        /// </summary>
        public static readonly ExpandedNodeId LifeType_WarningLevel = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.LifeType_WarningLevel, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the LifeType_Maximum Variable.
        /// </summary>
        public static readonly ExpandedNodeId LifeType_Maximum = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.LifeType_Maximum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the LocationType_Type Variable.
        /// </summary>
        public static readonly ExpandedNodeId LocationType_Type = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.LocationType_Type, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the LocationType_NegativeOverlap Variable.
        /// </summary>
        public static readonly ExpandedNodeId LocationType_NegativeOverlap = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.LocationType_NegativeOverlap, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the LocationType_PositiveOverlap Variable.
        /// </summary>
        public static readonly ExpandedNodeId LocationType_PositiveOverlap = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.LocationType_PositiveOverlap, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ProgramSpindleSpeedType_Maximum Variable.
        /// </summary>
        public static readonly ExpandedNodeId ProgramSpindleSpeedType_Maximum = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ProgramSpindleSpeedType_Maximum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ProgramSpindleSpeedType_Minimum Variable.
        /// </summary>
        public static readonly ExpandedNodeId ProgramSpindleSpeedType_Minimum = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ProgramSpindleSpeedType_Minimum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ProgramSpindleSpeedType_Nominal Variable.
        /// </summary>
        public static readonly ExpandedNodeId ProgramSpindleSpeedType_Nominal = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ProgramSpindleSpeedType_Nominal, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ProgramFeedRateType_Maximum Variable.
        /// </summary>
        public static readonly ExpandedNodeId ProgramFeedRateType_Maximum = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ProgramFeedRateType_Maximum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ProgramFeedRateType_Minimum Variable.
        /// </summary>
        public static readonly ExpandedNodeId ProgramFeedRateType_Minimum = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ProgramFeedRateType_Minimum, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ProgramFeedRateType_Nominal Variable.
        /// </summary>
        public static readonly ExpandedNodeId ProgramFeedRateType_Nominal = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ProgramFeedRateType_Nominal, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CuttingItemType_Indices Variable.
        /// </summary>
        public static readonly ExpandedNodeId CuttingItemType_Indices = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CuttingItemType_Indices, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CuttingItemType_ItemId Variable.
        /// </summary>
        public static readonly ExpandedNodeId CuttingItemType_ItemId = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CuttingItemType_ItemId, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CuttingItemType_Grade Variable.
        /// </summary>
        public static readonly ExpandedNodeId CuttingItemType_Grade = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CuttingItemType_Grade, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CuttingItemType_Manufacturers Variable.
        /// </summary>
        public static readonly ExpandedNodeId CuttingItemType_Manufacturers = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CuttingItemType_Manufacturers, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CuttingItemType_Description Variable.
        /// </summary>
        public static readonly ExpandedNodeId CuttingItemType_Description = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CuttingItemType_Description, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CuttingItemType_Locus Variable.
        /// </summary>
        public static readonly ExpandedNodeId CuttingItemType_Locus = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CuttingItemType_Locus, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ActuatorStateTypeEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId ActuatorStateTypeEnum_EnumValues = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ActuatorStateTypeEnum_EnumValues, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AlarmStateTypeEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId AlarmStateTypeEnum_EnumValues = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AlarmStateTypeEnum_EnumValues, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AvailabilityTypeEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId AvailabilityTypeEnum_EnumValues = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AvailabilityTypeEnum_EnumValues, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AxesCouplingTypeEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxesCouplingTypeEnum_EnumValues = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.AxesCouplingTypeEnum_EnumValues, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CategoryTypeEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId CategoryTypeEnum_EnumValues = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CategoryTypeEnum_EnumValues, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ClampStateTypeEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId ClampStateTypeEnum_EnumValues = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ClampStateTypeEnum_EnumValues, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ControllerModeTypeEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerModeTypeEnum_EnumValues = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ControllerModeTypeEnum_EnumValues, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CoordinateSystemTypeEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId CoordinateSystemTypeEnum_EnumValues = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CoordinateSystemTypeEnum_EnumValues, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CutterStatusValueTypeEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId CutterStatusValueTypeEnum_EnumValues = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.CutterStatusValueTypeEnum_EnumValues, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DataItemEnumTypeEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId DataItemEnumTypeEnum_EnumValues = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.DataItemEnumTypeEnum_EnumValues, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DataItemStatisticsTypeEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId DataItemStatisticsTypeEnum_EnumValues = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.DataItemStatisticsTypeEnum_EnumValues, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DataItemSubEnumTypeEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId DataItemSubEnumTypeEnum_EnumValues = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.DataItemSubEnumTypeEnum_EnumValues, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DefinitionFormatTypeEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId DefinitionFormatTypeEnum_EnumValues = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.DefinitionFormatTypeEnum_EnumValues, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DirectionTypeEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectionTypeEnum_EnumValues = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.DirectionTypeEnum_EnumValues, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DoorStateTypeEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId DoorStateTypeEnum_EnumValues = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.DoorStateTypeEnum_EnumValues, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the EmergencyStopTypeEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId EmergencyStopTypeEnum_EnumValues = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.EmergencyStopTypeEnum_EnumValues, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ExecutionTypeEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId ExecutionTypeEnum_EnumValues = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ExecutionTypeEnum_EnumValues, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the InterfaceStateTypeEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId InterfaceStateTypeEnum_EnumValues = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.InterfaceStateTypeEnum_EnumValues, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the LocationsTypeEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId LocationsTypeEnum_EnumValues = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.LocationsTypeEnum_EnumValues, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the NotificationCodeTypeEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId NotificationCodeTypeEnum_EnumValues = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.NotificationCodeTypeEnum_EnumValues, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PathModeTypeEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId PathModeTypeEnum_EnumValues = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PathModeTypeEnum_EnumValues, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PowerStateTypeEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerStateTypeEnum_EnumValues = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.PowerStateTypeEnum_EnumValues, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the QualifierTypeEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId QualifierTypeEnum_EnumValues = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.QualifierTypeEnum_EnumValues, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the RotaryModeTypeEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId RotaryModeTypeEnum_EnumValues = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.RotaryModeTypeEnum_EnumValues, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the SeverityTypeEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId SeverityTypeEnum_EnumValues = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.SeverityTypeEnum_EnumValues, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ToolLifeDirectionTypeEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId ToolLifeDirectionTypeEnum_EnumValues = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ToolLifeDirectionTypeEnum_EnumValues, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ToolLifeTypeEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId ToolLifeTypeEnum_EnumValues = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.ToolLifeTypeEnum_EnumValues, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnect_BinarySchema Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnect_BinarySchema = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.OpcUaMTConnect_BinarySchema, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnect_BinarySchema_NamespaceUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnect_BinarySchema_NamespaceUri = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.OpcUaMTConnect_BinarySchema_NamespaceUri, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnect_BinarySchema_Deprecated Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnect_BinarySchema_Deprecated = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.OpcUaMTConnect_BinarySchema_Deprecated, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnect_BinarySchema_DataItemEnumDataType Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnect_BinarySchema_DataItemEnumDataType = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.OpcUaMTConnect_BinarySchema_DataItemEnumDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnect_BinarySchema_DataItemStatisticsDataType Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnect_BinarySchema_DataItemStatisticsDataType = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.OpcUaMTConnect_BinarySchema_DataItemStatisticsDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnect_XmlSchema Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnect_XmlSchema = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.OpcUaMTConnect_XmlSchema, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnect_XmlSchema_NamespaceUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnect_XmlSchema_NamespaceUri = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.OpcUaMTConnect_XmlSchema_NamespaceUri, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnect_XmlSchema_Deprecated Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnect_XmlSchema_Deprecated = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.OpcUaMTConnect_XmlSchema_Deprecated, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnect_XmlSchema_DataItemEnumDataType Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnect_XmlSchema_DataItemEnumDataType = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.OpcUaMTConnect_XmlSchema_DataItemEnumDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the OpcUaMTConnect_XmlSchema_DataItemStatisticsDataType Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaMTConnect_XmlSchema_DataItemStatisticsDataType = new ExpandedNodeId(Opc.Ua.MTConnect.Variables.OpcUaMTConnect_XmlSchema_DataItemStatisticsDataType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);
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
        /// The identifier for the MTSampleDataItemType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId MTSampleDataItemType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.MTSampleDataItemType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AccelerationType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId AccelerationType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.AccelerationType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AccumulatedTimeType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId AccumulatedTimeType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.AccumulatedTimeType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AmperageType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId AmperageType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.AmperageType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AngleType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId AngleType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.AngleType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AngularAccelerationType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId AngularAccelerationType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.AngularAccelerationType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AngularVelocityType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId AngularVelocityType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.AngularVelocityType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AxisFeedrateType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId AxisFeedrateType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.AxisFeedrateType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ConcentrationType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId ConcentrationType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.ConcentrationType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ConductivityType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId ConductivityType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.ConductivityType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DisplacementType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId DisplacementType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.DisplacementType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ElectricalEnergyType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId ElectricalEnergyType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.ElectricalEnergyType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the FillLevelType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId FillLevelType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.FillLevelType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the FlowType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId FlowType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.FlowType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the FrequencyType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId FrequencyType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.FrequencyType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the LinearForceType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId LinearForceType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.LinearForceType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the LoadType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId LoadType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.LoadType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MassType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId MassType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.MassType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PathFeedrateType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId PathFeedrateType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.PathFeedrateType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PathPositionType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId PathPositionType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.PathPositionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PHType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId PHType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.PHType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PositionType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId PositionType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.PositionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PowerFactorType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId PowerFactorType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.PowerFactorType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PressureType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId PressureType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.PressureType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ResistanceType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId ResistanceType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.ResistanceType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the RotationalVelocityType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId RotationalVelocityType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.RotationalVelocityType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the SoundPressureType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId SoundPressureType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.SoundPressureType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the StrainType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId StrainType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.StrainType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TemperatureType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId TemperatureType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.TemperatureType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TiltType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId TiltType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.TiltType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the TorqueType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId TorqueType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.TorqueType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VelocityType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId VelocityType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.VelocityType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ViscosityType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId ViscosityType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.ViscosityType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VoltageType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId VoltageType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.VoltageType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VoltAmpereType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId VoltAmpereType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.VoltAmpereType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the VoltAmpereReactiveType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId VoltAmpereReactiveType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.VoltAmpereReactiveType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the WattageType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId WattageType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.WattageType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MTEventDataItemType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId MTEventDataItemType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.MTEventDataItemType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ActiveAxesType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId ActiveAxesType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.ActiveAxesType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ActuatorStateType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId ActuatorStateType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.ActuatorStateType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AvailabilityType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId AvailabilityType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.AvailabilityType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AxesCouplingType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId AxesCouplingType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.AxesCouplingType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the BlockType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId BlockType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.BlockType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ClampStateType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId ClampStateType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.ClampStateType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ControllerModeType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId ControllerModeType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.ControllerModeType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CoupledAxesType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId CoupledAxesType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.CoupledAxesType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DirectionType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId DirectionType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.DirectionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the DoorStateType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId DoorStateType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.DoorStateType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the EmergencyStopType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId EmergencyStopType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.EmergencyStopType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ExecutionType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId ExecutionType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.ExecutionType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the LineType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId LineType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.LineType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MessageType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId MessageType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.MessageType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PalletIdType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId PalletIdType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.PalletIdType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PartCountType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId PartCountType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.PartCountType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PartIdType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId PartIdType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.PartIdType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PathModeType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId PathModeType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.PathModeType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the PowerStateType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId PowerStateType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.PowerStateType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ProgramType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId ProgramType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.ProgramType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the RotaryModeType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId RotaryModeType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.RotaryModeType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ToolAssetIdType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId ToolAssetIdType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.ToolAssetIdType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ToolNumberType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId ToolNumberType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.ToolNumberType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the WorkHoldingIdType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId WorkHoldingIdType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.WorkHoldingIdType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ChannelType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId ChannelType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.ChannelType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the MeasurementType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId MeasurementType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.MeasurementType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CutterStatusType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId CutterStatusType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.CutterStatusType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ReconditionCountType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId ReconditionCountType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.ReconditionCountType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the LifeType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId LifeType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.LifeType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the LocationType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId LocationType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.LocationType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ProgramSpindleSpeedType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId ProgramSpindleSpeedType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.ProgramSpindleSpeedType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the ProgramFeedRateType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId ProgramFeedRateType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.ProgramFeedRateType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the CuttingItemType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId CuttingItemType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.CuttingItemType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);

        /// <summary>
        /// The identifier for the AssemblyMeasurementType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId AssemblyMeasurementType = new ExpandedNodeId(Opc.Ua.MTConnect.VariableTypes.AssemblyMeasurementType, Opc.Ua.MTConnect.Namespaces.OpcUaMTConnect);
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
        /// The BrowseName for the AccelerationConditionType component.
        /// </summary>
        public const string AccelerationConditionType = "AccelerationConditionType";

        /// <summary>
        /// The BrowseName for the AccelerationType component.
        /// </summary>
        public const string AccelerationType = "AccelerationType";

        /// <summary>
        /// The BrowseName for the Accumulated_TimeConditionType component.
        /// </summary>
        public const string Accumulated_TimeConditionType = "Accumulated_TimeConditionType";

        /// <summary>
        /// The BrowseName for the AccumulatedTimeType component.
        /// </summary>
        public const string AccumulatedTimeType = "AccumulatedTimeType";

        /// <summary>
        /// The BrowseName for the ActiveAxesType component.
        /// </summary>
        public const string ActiveAxesType = "ActiveAxesType";

        /// <summary>
        /// The BrowseName for the ActiveState component.
        /// </summary>
        public const string ActiveState = "ActiveState";

        /// <summary>
        /// The BrowseName for the ActuatorStateType component.
        /// </summary>
        public const string ActuatorStateType = "ActuatorStateType";

        /// <summary>
        /// The BrowseName for the ActuatorStateTypeEnum component.
        /// </summary>
        public const string ActuatorStateTypeEnum = "ActuatorStateTypeEnum";

        /// <summary>
        /// The BrowseName for the AlarmStateTypeEnum component.
        /// </summary>
        public const string AlarmStateTypeEnum = "AlarmStateTypeEnum";

        /// <summary>
        /// The BrowseName for the AmperageConditionType component.
        /// </summary>
        public const string AmperageConditionType = "AmperageConditionType";

        /// <summary>
        /// The BrowseName for the AmperageType component.
        /// </summary>
        public const string AmperageType = "AmperageType";

        /// <summary>
        /// The BrowseName for the AngleConditionType component.
        /// </summary>
        public const string AngleConditionType = "AngleConditionType";

        /// <summary>
        /// The BrowseName for the AngleType component.
        /// </summary>
        public const string AngleType = "AngleType";

        /// <summary>
        /// The BrowseName for the Angular_VelocityConditionType component.
        /// </summary>
        public const string Angular_VelocityConditionType = "Angular_VelocityConditionType";

        /// <summary>
        /// The BrowseName for the Angular-AccelerationConditionType component.
        /// </summary>
        public const string Angular-AccelerationConditionType = "Angular-AccelerationConditionType";

        /// <summary>
        /// The BrowseName for the AngularAccelerationType component.
        /// </summary>
        public const string AngularAccelerationType = "AngularAccelerationType";

        /// <summary>
        /// The BrowseName for the AngularVelocityType component.
        /// </summary>
        public const string AngularVelocityType = "AngularVelocityType";

        /// <summary>
        /// The BrowseName for the AssemblyMeasurementType component.
        /// </summary>
        public const string AssemblyMeasurementType = "AssemblyMeasurementType";

        /// <summary>
        /// The BrowseName for the AssetAttrDataType component.
        /// </summary>
        public const string AssetAttrDataType = "AssetAttrDataType";

        /// <summary>
        /// The BrowseName for the AssetBufferSizeDataType component.
        /// </summary>
        public const string AssetBufferSizeDataType = "AssetBufferSizeDataType";

        /// <summary>
        /// The BrowseName for the AssetDescription component.
        /// </summary>
        public const string AssetDescription = "AssetDescription";

        /// <summary>
        /// The BrowseName for the AssetDescriptionType component.
        /// </summary>
        public const string AssetDescriptionType = "AssetDescriptionType";

        /// <summary>
        /// The BrowseName for the AssetIdDataType component.
        /// </summary>
        public const string AssetIdDataType = "AssetIdDataType";

        /// <summary>
        /// The BrowseName for the AssetType component.
        /// </summary>
        public const string AssetType = "AssetType";

        /// <summary>
        /// The BrowseName for the Availability component.
        /// </summary>
        public const string Availability = "Availability";

        /// <summary>
        /// The BrowseName for the AvailabilityType component.
        /// </summary>
        public const string AvailabilityType = "AvailabilityType";

        /// <summary>
        /// The BrowseName for the AvailabilityTypeEnum component.
        /// </summary>
        public const string AvailabilityTypeEnum = "AvailabilityTypeEnum";

        /// <summary>
        /// The BrowseName for the AxesCouplingType component.
        /// </summary>
        public const string AxesCouplingType = "AxesCouplingType";

        /// <summary>
        /// The BrowseName for the AxesCouplingTypeEnum component.
        /// </summary>
        public const string AxesCouplingTypeEnum = "AxesCouplingTypeEnum";

        /// <summary>
        /// The BrowseName for the AxesListValueDataType component.
        /// </summary>
        public const string AxesListValueDataType = "AxesListValueDataType";

        /// <summary>
        /// The BrowseName for the AxisFeedrateType component.
        /// </summary>
        public const string AxisFeedrateType = "AxisFeedrateType";

        /// <summary>
        /// The BrowseName for the BlockType component.
        /// </summary>
        public const string BlockType = "BlockType";

        /// <summary>
        /// The BrowseName for the BufferSizeDataType component.
        /// </summary>
        public const string BufferSizeDataType = "BufferSizeDataType";

        /// <summary>
        /// The BrowseName for the CalibrationDate component.
        /// </summary>
        public const string CalibrationDate = "CalibrationDate";

        /// <summary>
        /// The BrowseName for the CalibrationDateDataType component.
        /// </summary>
        public const string CalibrationDateDataType = "CalibrationDateDataType";

        /// <summary>
        /// The BrowseName for the CalibrationInitials component.
        /// </summary>
        public const string CalibrationInitials = "CalibrationInitials";

        /// <summary>
        /// The BrowseName for the CalibrationInitialsDataType component.
        /// </summary>
        public const string CalibrationInitialsDataType = "CalibrationInitialsDataType";

        /// <summary>
        /// The BrowseName for the CategoryTypeEnum component.
        /// </summary>
        public const string CategoryTypeEnum = "CategoryTypeEnum";

        /// <summary>
        /// The BrowseName for the ChannelNumberDataType component.
        /// </summary>
        public const string ChannelNumberDataType = "ChannelNumberDataType";

        /// <summary>
        /// The BrowseName for the ChannelType component.
        /// </summary>
        public const string ChannelType = "ChannelType";

        /// <summary>
        /// The BrowseName for the ClampStateType component.
        /// </summary>
        public const string ClampStateType = "ClampStateType";

        /// <summary>
        /// The BrowseName for the ClampStateTypeEnum component.
        /// </summary>
        public const string ClampStateTypeEnum = "ClampStateTypeEnum";

        /// <summary>
        /// The BrowseName for the Code component.
        /// </summary>
        public const string Code = "Code";

        /// <summary>
        /// The BrowseName for the CodeDataType component.
        /// </summary>
        public const string CodeDataType = "CodeDataType";

        /// <summary>
        /// The BrowseName for the CommunicationsConditionType component.
        /// </summary>
        public const string CommunicationsConditionType = "CommunicationsConditionType";

        /// <summary>
        /// The BrowseName for the ComponentId component.
        /// </summary>
        public const string ComponentId = "ComponentId";

        /// <summary>
        /// The BrowseName for the ComponentIdDataType component.
        /// </summary>
        public const string ComponentIdDataType = "ComponentIdDataType";

        /// <summary>
        /// The BrowseName for the Components component.
        /// </summary>
        public const string Components = "Components";

        /// <summary>
        /// The BrowseName for the ConcentrationConditionType component.
        /// </summary>
        public const string ConcentrationConditionType = "ConcentrationConditionType";

        /// <summary>
        /// The BrowseName for the ConcentrationType component.
        /// </summary>
        public const string ConcentrationType = "ConcentrationType";

        /// <summary>
        /// The BrowseName for the ConditionDescriptionDataType component.
        /// </summary>
        public const string ConditionDescriptionDataType = "ConditionDescriptionDataType";

        /// <summary>
        /// The BrowseName for the Conditions component.
        /// </summary>
        public const string Conditions = "Conditions";

        /// <summary>
        /// The BrowseName for the ConductivityConditionType component.
        /// </summary>
        public const string ConductivityConditionType = "ConductivityConditionType";

        /// <summary>
        /// The BrowseName for the ConductivityType component.
        /// </summary>
        public const string ConductivityType = "ConductivityType";

        /// <summary>
        /// The BrowseName for the Configuration component.
        /// </summary>
        public const string Configuration = "Configuration";

        /// <summary>
        /// The BrowseName for the ConfigurationType component.
        /// </summary>
        public const string ConfigurationType = "ConfigurationType";

        /// <summary>
        /// The BrowseName for the ConnectionCodeMachineSide component.
        /// </summary>
        public const string ConnectionCodeMachineSide = "ConnectionCodeMachineSide";

        /// <summary>
        /// The BrowseName for the ConnectionCodeMachineSideDataType component.
        /// </summary>
        public const string ConnectionCodeMachineSideDataType = "ConnectionCodeMachineSideDataType";

        /// <summary>
        /// The BrowseName for the ControllerModeType component.
        /// </summary>
        public const string ControllerModeType = "ControllerModeType";

        /// <summary>
        /// The BrowseName for the ControllerModeTypeEnum component.
        /// </summary>
        public const string ControllerModeTypeEnum = "ControllerModeTypeEnum";

        /// <summary>
        /// The BrowseName for the CoordinateSystem component.
        /// </summary>
        public const string CoordinateSystem = "CoordinateSystem";

        /// <summary>
        /// The BrowseName for the CoordinateSystemTypeEnum component.
        /// </summary>
        public const string CoordinateSystemTypeEnum = "CoordinateSystemTypeEnum";

        /// <summary>
        /// The BrowseName for the CountDirection component.
        /// </summary>
        public const string CountDirection = "CountDirection";

        /// <summary>
        /// The BrowseName for the CountValueDataType component.
        /// </summary>
        public const string CountValueDataType = "CountValueDataType";

        /// <summary>
        /// The BrowseName for the CoupledAxesType component.
        /// </summary>
        public const string CoupledAxesType = "CoupledAxesType";

        /// <summary>
        /// The BrowseName for the CreationTimeDataType component.
        /// </summary>
        public const string CreationTimeDataType = "CreationTimeDataType";

        /// <summary>
        /// The BrowseName for the CutterStatus component.
        /// </summary>
        public const string CutterStatus = "CutterStatus";

        /// <summary>
        /// The BrowseName for the CutterStatusType component.
        /// </summary>
        public const string CutterStatusType = "CutterStatusType";

        /// <summary>
        /// The BrowseName for the CutterStatusValueTypeEnum component.
        /// </summary>
        public const string CutterStatusValueTypeEnum = "CutterStatusValueTypeEnum";

        /// <summary>
        /// The BrowseName for the CuttingItemType component.
        /// </summary>
        public const string CuttingItemType = "CuttingItemType";

        /// <summary>
        /// The BrowseName for the CuttingToolLifeCycleType component.
        /// </summary>
        public const string CuttingToolLifeCycleType = "CuttingToolLifeCycleType";

        /// <summary>
        /// The BrowseName for the CuttingToolType component.
        /// </summary>
        public const string CuttingToolType = "CuttingToolType";

        /// <summary>
        /// The BrowseName for the Data_RangeConditionType component.
        /// </summary>
        public const string Data_RangeConditionType = "Data_RangeConditionType";

        /// <summary>
        /// The BrowseName for the DataItemEnumDataType component.
        /// </summary>
        public const string DataItemEnumDataType = "DataItemEnumDataType";

        /// <summary>
        /// The BrowseName for the DataItemEnumTypeEnum component.
        /// </summary>
        public const string DataItemEnumTypeEnum = "DataItemEnumTypeEnum";

        /// <summary>
        /// The BrowseName for the DataItemIdDataType component.
        /// </summary>
        public const string DataItemIdDataType = "DataItemIdDataType";

        /// <summary>
        /// The BrowseName for the DataItems component.
        /// </summary>
        public const string DataItems = "DataItems";

        /// <summary>
        /// The BrowseName for the DataItemStatisticsDataType component.
        /// </summary>
        public const string DataItemStatisticsDataType = "DataItemStatisticsDataType";

        /// <summary>
        /// The BrowseName for the DataItemStatisticsTypeEnum component.
        /// </summary>
        public const string DataItemStatisticsTypeEnum = "DataItemStatisticsTypeEnum";

        /// <summary>
        /// The BrowseName for the DataItemSubEnumTypeEnum component.
        /// </summary>
        public const string DataItemSubEnumTypeEnum = "DataItemSubEnumTypeEnum";

        /// <summary>
        /// The BrowseName for the DecibelValueDataType component.
        /// </summary>
        public const string DecibelValueDataType = "DecibelValueDataType";

        /// <summary>
        /// The BrowseName for the DefinitionFormatTypeEnum component.
        /// </summary>
        public const string DefinitionFormatTypeEnum = "DefinitionFormatTypeEnum";

        /// <summary>
        /// The BrowseName for the Description component.
        /// </summary>
        public const string Description = "Description";

        /// <summary>
        /// The BrowseName for the DescriptionTextDataType component.
        /// </summary>
        public const string DescriptionTextDataType = "DescriptionTextDataType";

        /// <summary>
        /// The BrowseName for the DirectionConditionType component.
        /// </summary>
        public const string DirectionConditionType = "DirectionConditionType";

        /// <summary>
        /// The BrowseName for the DirectionType component.
        /// </summary>
        public const string DirectionType = "DirectionType";

        /// <summary>
        /// The BrowseName for the DirectionTypeEnum component.
        /// </summary>
        public const string DirectionTypeEnum = "DirectionTypeEnum";

        /// <summary>
        /// The BrowseName for the DisplacementConditionType component.
        /// </summary>
        public const string DisplacementConditionType = "DisplacementConditionType";

        /// <summary>
        /// The BrowseName for the DisplacementType component.
        /// </summary>
        public const string DisplacementType = "DisplacementType";

        /// <summary>
        /// The BrowseName for the DoorState component.
        /// </summary>
        public const string DoorState = "DoorState";

        /// <summary>
        /// The BrowseName for the DoorStateType component.
        /// </summary>
        public const string DoorStateType = "DoorStateType";

        /// <summary>
        /// The BrowseName for the DoorStateTypeEnum component.
        /// </summary>
        public const string DoorStateTypeEnum = "DoorStateTypeEnum";

        /// <summary>
        /// The BrowseName for the DurationTimeDataType component.
        /// </summary>
        public const string DurationTimeDataType = "DurationTimeDataType";

        /// <summary>
        /// The BrowseName for the DurationValueDataType component.
        /// </summary>
        public const string DurationValueDataType = "DurationValueDataType";

        /// <summary>
        /// The BrowseName for the EdgeCountDataType component.
        /// </summary>
        public const string EdgeCountDataType = "EdgeCountDataType";

        /// <summary>
        /// The BrowseName for the Electrical_EnergyConditionType component.
        /// </summary>
        public const string Electrical_EnergyConditionType = "Electrical_EnergyConditionType";

        /// <summary>
        /// The BrowseName for the ElectricalEnergyType component.
        /// </summary>
        public const string ElectricalEnergyType = "ElectricalEnergyType";

        /// <summary>
        /// The BrowseName for the EmergencyStopType component.
        /// </summary>
        public const string EmergencyStopType = "EmergencyStopType";

        /// <summary>
        /// The BrowseName for the EmergencyStopTypeEnum component.
        /// </summary>
        public const string EmergencyStopTypeEnum = "EmergencyStopTypeEnum";

        /// <summary>
        /// The BrowseName for the EnergyValueDataType component.
        /// </summary>
        public const string EnergyValueDataType = "EnergyValueDataType";

        /// <summary>
        /// The BrowseName for the ExecutionType component.
        /// </summary>
        public const string ExecutionType = "ExecutionType";

        /// <summary>
        /// The BrowseName for the ExecutionTypeEnum component.
        /// </summary>
        public const string ExecutionTypeEnum = "ExecutionTypeEnum";

        /// <summary>
        /// The BrowseName for the FeedrateValueDataType component.
        /// </summary>
        public const string FeedrateValueDataType = "FeedrateValueDataType";

        /// <summary>
        /// The BrowseName for the Fill_LevelConditionType component.
        /// </summary>
        public const string Fill_LevelConditionType = "Fill_LevelConditionType";

        /// <summary>
        /// The BrowseName for the FillLevelType component.
        /// </summary>
        public const string FillLevelType = "FillLevelType";

        /// <summary>
        /// The BrowseName for the FirmwareVersion component.
        /// </summary>
        public const string FirmwareVersion = "FirmwareVersion";

        /// <summary>
        /// The BrowseName for the FirmwareVersionDataType component.
        /// </summary>
        public const string FirmwareVersionDataType = "FirmwareVersionDataType";

        /// <summary>
        /// The BrowseName for the FlowConditionType component.
        /// </summary>
        public const string FlowConditionType = "FlowConditionType";

        /// <summary>
        /// The BrowseName for the FlowType component.
        /// </summary>
        public const string FlowType = "FlowType";

        /// <summary>
        /// The BrowseName for the ForceValueDataType component.
        /// </summary>
        public const string ForceValueDataType = "ForceValueDataType";

        /// <summary>
        /// The BrowseName for the FrequencyConditionType component.
        /// </summary>
        public const string FrequencyConditionType = "FrequencyConditionType";

        /// <summary>
        /// The BrowseName for the FrequencyType component.
        /// </summary>
        public const string FrequencyType = "FrequencyType";

        /// <summary>
        /// The BrowseName for the Grade component.
        /// </summary>
        public const string Grade = "Grade";

        /// <summary>
        /// The BrowseName for the GradeDataType component.
        /// </summary>
        public const string GradeDataType = "GradeDataType";

        /// <summary>
        /// The BrowseName for the HardwareConditionType component.
        /// </summary>
        public const string HardwareConditionType = "HardwareConditionType";

        /// <summary>
        /// The BrowseName for the IDDataType component.
        /// </summary>
        public const string IDDataType = "IDDataType";

        /// <summary>
        /// The BrowseName for the IndexRangeDataType component.
        /// </summary>
        public const string IndexRangeDataType = "IndexRangeDataType";

        /// <summary>
        /// The BrowseName for the Indices component.
        /// </summary>
        public const string Indices = "Indices";

        /// <summary>
        /// The BrowseName for the InstanceIdDataType component.
        /// </summary>
        public const string InstanceIdDataType = "InstanceIdDataType";

        /// <summary>
        /// The BrowseName for the InterfaceStateTypeEnum component.
        /// </summary>
        public const string InterfaceStateTypeEnum = "InterfaceStateTypeEnum";

        /// <summary>
        /// The BrowseName for the ItemId component.
        /// </summary>
        public const string ItemId = "ItemId";

        /// <summary>
        /// The BrowseName for the ItemIdDataType component.
        /// </summary>
        public const string ItemIdDataType = "ItemIdDataType";

        /// <summary>
        /// The BrowseName for the ItemLife component.
        /// </summary>
        public const string ItemLife = "ItemLife";

        /// <summary>
        /// The BrowseName for the ItemSourceDataType component.
        /// </summary>
        public const string ItemSourceDataType = "ItemSourceDataType";

        /// <summary>
        /// The BrowseName for the LastChangeTimestamp component.
        /// </summary>
        public const string LastChangeTimestamp = "LastChangeTimestamp";

        /// <summary>
        /// The BrowseName for the LifeType component.
        /// </summary>
        public const string LifeType = "LifeType";

        /// <summary>
        /// The BrowseName for the LimitState component.
        /// </summary>
        public const string LimitState = "LimitState";

        /// <summary>
        /// The BrowseName for the Linear_ForceConditionType component.
        /// </summary>
        public const string Linear_ForceConditionType = "Linear_ForceConditionType";

        /// <summary>
        /// The BrowseName for the LinearAxesX component.
        /// </summary>
        public const string LinearAxesX = "LinearAxesX";

        /// <summary>
        /// The BrowseName for the LinearAxesY component.
        /// </summary>
        public const string LinearAxesY = "LinearAxesY";

        /// <summary>
        /// The BrowseName for the LinearAxesZ component.
        /// </summary>
        public const string LinearAxesZ = "LinearAxesZ";

        /// <summary>
        /// The BrowseName for the LinearForceType component.
        /// </summary>
        public const string LinearForceType = "LinearForceType";

        /// <summary>
        /// The BrowseName for the LineType component.
        /// </summary>
        public const string LineType = "LineType";

        /// <summary>
        /// The BrowseName for the LoadConditionType component.
        /// </summary>
        public const string LoadConditionType = "LoadConditionType";

        /// <summary>
        /// The BrowseName for the LoadType component.
        /// </summary>
        public const string LoadType = "LoadType";

        /// <summary>
        /// The BrowseName for the Location component.
        /// </summary>
        public const string Location = "Location";

        /// <summary>
        /// The BrowseName for the LocationSizeDataType component.
        /// </summary>
        public const string LocationSizeDataType = "LocationSizeDataType";

        /// <summary>
        /// The BrowseName for the LocationsTypeEnum component.
        /// </summary>
        public const string LocationsTypeEnum = "LocationsTypeEnum";

        /// <summary>
        /// The BrowseName for the LocationType component.
        /// </summary>
        public const string LocationType = "LocationType";

        /// <summary>
        /// The BrowseName for the LocationValueDataType component.
        /// </summary>
        public const string LocationValueDataType = "LocationValueDataType";

        /// <summary>
        /// The BrowseName for the Locus component.
        /// </summary>
        public const string Locus = "Locus";

        /// <summary>
        /// The BrowseName for the LocusDataType component.
        /// </summary>
        public const string LocusDataType = "LocusDataType";

        /// <summary>
        /// The BrowseName for the Logic_ProgramConditionType component.
        /// </summary>
        public const string Logic_ProgramConditionType = "Logic_ProgramConditionType";

        /// <summary>
        /// The BrowseName for the Manufacturer component.
        /// </summary>
        public const string Manufacturer = "Manufacturer";

        /// <summary>
        /// The BrowseName for the Manufacturers component.
        /// </summary>
        public const string Manufacturers = "Manufacturers";

        /// <summary>
        /// The BrowseName for the ManufacturersDataType component.
        /// </summary>
        public const string ManufacturersDataType = "ManufacturersDataType";

        /// <summary>
        /// The BrowseName for the MassConditionType component.
        /// </summary>
        public const string MassConditionType = "MassConditionType";

        /// <summary>
        /// The BrowseName for the MassType component.
        /// </summary>
        public const string MassType = "MassType";

        /// <summary>
        /// The BrowseName for the MassValueDataType component.
        /// </summary>
        public const string MassValueDataType = "MassValueDataType";

        /// <summary>
        /// The BrowseName for the Maximum component.
        /// </summary>
        public const string Maximum = "Maximum";

        /// <summary>
        /// The BrowseName for the MaximumCount component.
        /// </summary>
        public const string MaximumCount = "MaximumCount";

        /// <summary>
        /// The BrowseName for the MaximumCountDataType component.
        /// </summary>
        public const string MaximumCountDataType = "MaximumCountDataType";

        /// <summary>
        /// The BrowseName for the MaximumDataType component.
        /// </summary>
        public const string MaximumDataType = "MaximumDataType";

        /// <summary>
        /// The BrowseName for the Measurements component.
        /// </summary>
        public const string Measurements = "Measurements";

        /// <summary>
        /// The BrowseName for the MeasurementType component.
        /// </summary>
        public const string MeasurementType = "MeasurementType";

        /// <summary>
        /// The BrowseName for the MeasurementValueDataType component.
        /// </summary>
        public const string MeasurementValueDataType = "MeasurementValueDataType";

        /// <summary>
        /// The BrowseName for the MessageType component.
        /// </summary>
        public const string MessageType = "MessageType";

        /// <summary>
        /// The BrowseName for the Minimum component.
        /// </summary>
        public const string Minimum = "Minimum";

        /// <summary>
        /// The BrowseName for the MinimumDataType component.
        /// </summary>
        public const string MinimumDataType = "MinimumDataType";

        /// <summary>
        /// The BrowseName for the Model component.
        /// </summary>
        public const string Model = "Model";

        /// <summary>
        /// The BrowseName for the ModelDataType component.
        /// </summary>
        public const string ModelDataType = "ModelDataType";

        /// <summary>
        /// The BrowseName for the Motion_ProgramConditionType component.
        /// </summary>
        public const string Motion_ProgramConditionType = "Motion_ProgramConditionType";

        /// <summary>
        /// The BrowseName for the MTActuatorType component.
        /// </summary>
        public const string MTActuatorType = "MTActuatorType";

        /// <summary>
        /// The BrowseName for the MTAxesType component.
        /// </summary>
        public const string MTAxesType = "MTAxesType";

        /// <summary>
        /// The BrowseName for the MTComponentType component.
        /// </summary>
        public const string MTComponentType = "MTComponentType";

        /// <summary>
        /// The BrowseName for the MTConditionType component.
        /// </summary>
        public const string MTConditionType = "MTConditionType";

        /// <summary>
        /// The BrowseName for the MTConnectVersion component.
        /// </summary>
        public const string MTConnectVersion = "MTConnectVersion";

        /// <summary>
        /// The BrowseName for the MTControllerType component.
        /// </summary>
        public const string MTControllerType = "MTControllerType";

        /// <summary>
        /// The BrowseName for the MTCurrentState component.
        /// </summary>
        public const string MTCurrentState = "MTCurrentState";

        /// <summary>
        /// The BrowseName for the MTDevices component.
        /// </summary>
        public const string MTDevices = "MTDevices";

        /// <summary>
        /// The BrowseName for the MTDevicesType component.
        /// </summary>
        public const string MTDevicesType = "MTDevicesType";

        /// <summary>
        /// The BrowseName for the MTDeviceType component.
        /// </summary>
        public const string MTDeviceType = "MTDeviceType";

        /// <summary>
        /// The BrowseName for the MTDoorType component.
        /// </summary>
        public const string MTDoorType = "MTDoorType";

        /// <summary>
        /// The BrowseName for the MTEventDataItemType component.
        /// </summary>
        public const string MTEventDataItemType = "MTEventDataItemType";

        /// <summary>
        /// The BrowseName for the MTSampleDataItemType component.
        /// </summary>
        public const string MTSampleDataItemType = "MTSampleDataItemType";

        /// <summary>
        /// The BrowseName for the NameDataType component.
        /// </summary>
        public const string NameDataType = "NameDataType";

        /// <summary>
        /// The BrowseName for the NativeCode component.
        /// </summary>
        public const string NativeCode = "NativeCode";

        /// <summary>
        /// The BrowseName for the NativeCodeDataType component.
        /// </summary>
        public const string NativeCodeDataType = "NativeCodeDataType";

        /// <summary>
        /// The BrowseName for the NativeNotifcationCodeDataType component.
        /// </summary>
        public const string NativeNotifcationCodeDataType = "NativeNotifcationCodeDataType";

        /// <summary>
        /// The BrowseName for the NativeScale component.
        /// </summary>
        public const string NativeScale = "NativeScale";

        /// <summary>
        /// The BrowseName for the NativeScaleDataType component.
        /// </summary>
        public const string NativeScaleDataType = "NativeScaleDataType";

        /// <summary>
        /// The BrowseName for the NativeSeverity component.
        /// </summary>
        public const string NativeSeverity = "NativeSeverity";

        /// <summary>
        /// The BrowseName for the NativeSeverityDataType component.
        /// </summary>
        public const string NativeSeverityDataType = "NativeSeverityDataType";

        /// <summary>
        /// The BrowseName for the NativeUnits component.
        /// </summary>
        public const string NativeUnits = "NativeUnits";

        /// <summary>
        /// The BrowseName for the NegativeOverlap component.
        /// </summary>
        public const string NegativeOverlap = "NegativeOverlap";

        /// <summary>
        /// The BrowseName for the NextCalibrationDate component.
        /// </summary>
        public const string NextCalibrationDate = "NextCalibrationDate";

        /// <summary>
        /// The BrowseName for the NextCalibrationDateDataType component.
        /// </summary>
        public const string NextCalibrationDateDataType = "NextCalibrationDateDataType";

        /// <summary>
        /// The BrowseName for the Nominal component.
        /// </summary>
        public const string Nominal = "Nominal";

        /// <summary>
        /// The BrowseName for the NominalDataType component.
        /// </summary>
        public const string NominalDataType = "NominalDataType";

        /// <summary>
        /// The BrowseName for the NotificationCodeTypeEnum component.
        /// </summary>
        public const string NotificationCodeTypeEnum = "NotificationCodeTypeEnum";

        /// <summary>
        /// The BrowseName for the NotificationDescriptionDataType component.
        /// </summary>
        public const string NotificationDescriptionDataType = "NotificationDescriptionDataType";

        /// <summary>
        /// The BrowseName for the OccurrenceTimeDataType component.
        /// </summary>
        public const string OccurrenceTimeDataType = "OccurrenceTimeDataType";

        /// <summary>
        /// The BrowseName for the OPCUAMappingDate component.
        /// </summary>
        public const string OPCUAMappingDate = "OPCUAMappingDate";

        /// <summary>
        /// The BrowseName for the OpcUaMTConnect_BinarySchema component.
        /// </summary>
        public const string OpcUaMTConnect_BinarySchema = "Opc.Ua.MTConnect";

        /// <summary>
        /// The BrowseName for the OpcUaMTConnect_XmlSchema component.
        /// </summary>
        public const string OpcUaMTConnect_XmlSchema = "Opc.Ua.MTConnect";

        /// <summary>
        /// The BrowseName for the OpcUaMTConnectNamespaceMetadata component.
        /// </summary>
        public const string OpcUaMTConnectNamespaceMetadata = "http://opcfoundation.org/UA/MTConnect/";

        /// <summary>
        /// The BrowseName for the OPCUAVersion component.
        /// </summary>
        public const string OPCUAVersion = "OPCUAVersion";

        /// <summary>
        /// The BrowseName for the OverlapDataType component.
        /// </summary>
        public const string OverlapDataType = "OverlapDataType";

        /// <summary>
        /// The BrowseName for the PalletIdType component.
        /// </summary>
        public const string PalletIdType = "PalletIdType";

        /// <summary>
        /// The BrowseName for the PartCountType component.
        /// </summary>
        public const string PartCountType = "PartCountType";

        /// <summary>
        /// The BrowseName for the PartIdType component.
        /// </summary>
        public const string PartIdType = "PartIdType";

        /// <summary>
        /// The BrowseName for the Path_FeedrateConditionType component.
        /// </summary>
        public const string Path_FeedrateConditionType = "Path_FeedrateConditionType";

        /// <summary>
        /// The BrowseName for the Path_PositionConditionType component.
        /// </summary>
        public const string Path_PositionConditionType = "Path_PositionConditionType";

        /// <summary>
        /// The BrowseName for the PathFeedrateType component.
        /// </summary>
        public const string PathFeedrateType = "PathFeedrateType";

        /// <summary>
        /// The BrowseName for the PathModeType component.
        /// </summary>
        public const string PathModeType = "PathModeType";

        /// <summary>
        /// The BrowseName for the PathModeTypeEnum component.
        /// </summary>
        public const string PathModeTypeEnum = "PathModeTypeEnum";

        /// <summary>
        /// The BrowseName for the PathPositionType component.
        /// </summary>
        public const string PathPositionType = "PathPositionType";

        /// <summary>
        /// The BrowseName for the PHConditionType component.
        /// </summary>
        public const string PHConditionType = "PHConditionType";

        /// <summary>
        /// The BrowseName for the PHType component.
        /// </summary>
        public const string PHType = "PHType";

        /// <summary>
        /// The BrowseName for the PositionConditionType component.
        /// </summary>
        public const string PositionConditionType = "PositionConditionType";

        /// <summary>
        /// The BrowseName for the PositionType component.
        /// </summary>
        public const string PositionType = "PositionType";

        /// <summary>
        /// The BrowseName for the PositiveOverlap component.
        /// </summary>
        public const string PositiveOverlap = "PositiveOverlap";

        /// <summary>
        /// The BrowseName for the Power_FactorConditionType component.
        /// </summary>
        public const string Power_FactorConditionType = "Power_FactorConditionType";

        /// <summary>
        /// The BrowseName for the PowerFactorType component.
        /// </summary>
        public const string PowerFactorType = "PowerFactorType";

        /// <summary>
        /// The BrowseName for the PowerStateType component.
        /// </summary>
        public const string PowerStateType = "PowerStateType";

        /// <summary>
        /// The BrowseName for the PowerStateTypeEnum component.
        /// </summary>
        public const string PowerStateTypeEnum = "PowerStateTypeEnum";

        /// <summary>
        /// The BrowseName for the PressureConditionType component.
        /// </summary>
        public const string PressureConditionType = "PressureConditionType";

        /// <summary>
        /// The BrowseName for the PressureType component.
        /// </summary>
        public const string PressureType = "PressureType";

        /// <summary>
        /// The BrowseName for the ProgramFeedRate component.
        /// </summary>
        public const string ProgramFeedRate = "ProgramFeedRate";

        /// <summary>
        /// The BrowseName for the ProgramFeedRateType component.
        /// </summary>
        public const string ProgramFeedRateType = "ProgramFeedRateType";

        /// <summary>
        /// The BrowseName for the ProgramSpindleSpeed component.
        /// </summary>
        public const string ProgramSpindleSpeed = "ProgramSpindleSpeed";

        /// <summary>
        /// The BrowseName for the ProgramSpindleSpeedType component.
        /// </summary>
        public const string ProgramSpindleSpeedType = "ProgramSpindleSpeedType";

        /// <summary>
        /// The BrowseName for the ProgramToolNumber component.
        /// </summary>
        public const string ProgramToolNumber = "ProgramToolNumber";

        /// <summary>
        /// The BrowseName for the ProgramToolNumberDataType component.
        /// </summary>
        public const string ProgramToolNumberDataType = "ProgramToolNumberDataType";

        /// <summary>
        /// The BrowseName for the ProgramType component.
        /// </summary>
        public const string ProgramType = "ProgramType";

        /// <summary>
        /// The BrowseName for the QualifierTypeEnum component.
        /// </summary>
        public const string QualifierTypeEnum = "QualifierTypeEnum";

        /// <summary>
        /// The BrowseName for the RateDataType component.
        /// </summary>
        public const string RateDataType = "RateDataType";

        /// <summary>
        /// The BrowseName for the ReconditionCount component.
        /// </summary>
        public const string ReconditionCount = "ReconditionCount";

        /// <summary>
        /// The BrowseName for the ReconditionCountType component.
        /// </summary>
        public const string ReconditionCountType = "ReconditionCountType";

        /// <summary>
        /// The BrowseName for the ReconditionCountValueDataType component.
        /// </summary>
        public const string ReconditionCountValueDataType = "ReconditionCountValueDataType";

        /// <summary>
        /// The BrowseName for the ResistanceConditionType component.
        /// </summary>
        public const string ResistanceConditionType = "ResistanceConditionType";

        /// <summary>
        /// The BrowseName for the ResistanceType component.
        /// </summary>
        public const string ResistanceType = "ResistanceType";

        /// <summary>
        /// The BrowseName for the Rotary_VelocityConditionType component.
        /// </summary>
        public const string Rotary_VelocityConditionType = "Rotary_VelocityConditionType";

        /// <summary>
        /// The BrowseName for the RotaryModeType component.
        /// </summary>
        public const string RotaryModeType = "RotaryModeType";

        /// <summary>
        /// The BrowseName for the RotaryModeTypeEnum component.
        /// </summary>
        public const string RotaryModeTypeEnum = "RotaryModeTypeEnum";

        /// <summary>
        /// The BrowseName for the RotationalVelocityType component.
        /// </summary>
        public const string RotationalVelocityType = "RotationalVelocityType";

        /// <summary>
        /// The BrowseName for the RotoryAxesA component.
        /// </summary>
        public const string RotoryAxesA = "RotoryAxesA";

        /// <summary>
        /// The BrowseName for the RotoryAxesB component.
        /// </summary>
        public const string RotoryAxesB = "RotoryAxesB";

        /// <summary>
        /// The BrowseName for the RotoryAxesC component.
        /// </summary>
        public const string RotoryAxesC = "RotoryAxesC";

        /// <summary>
        /// The BrowseName for the SampleInterval component.
        /// </summary>
        public const string SampleInterval = "SampleInterval";

        /// <summary>
        /// The BrowseName for the SampleRateDataType component.
        /// </summary>
        public const string SampleRateDataType = "SampleRateDataType";

        /// <summary>
        /// The BrowseName for the SenderDataType component.
        /// </summary>
        public const string SenderDataType = "SenderDataType";

        /// <summary>
        /// The BrowseName for the SensorConfigurationType component.
        /// </summary>
        public const string SensorConfigurationType = "SensorConfigurationType";

        /// <summary>
        /// The BrowseName for the SensorType component.
        /// </summary>
        public const string SensorType = "SensorType";

        /// <summary>
        /// The BrowseName for the SequenceDataType component.
        /// </summary>
        public const string SequenceDataType = "SequenceDataType";

        /// <summary>
        /// The BrowseName for the SerialNumber component.
        /// </summary>
        public const string SerialNumber = "SerialNumber";

        /// <summary>
        /// The BrowseName for the SerialNumberDataType component.
        /// </summary>
        public const string SerialNumberDataType = "SerialNumberDataType";

        /// <summary>
        /// The BrowseName for the SeverityTypeEnum component.
        /// </summary>
        public const string SeverityTypeEnum = "SeverityTypeEnum";

        /// <summary>
        /// The BrowseName for the SignificantDigits component.
        /// </summary>
        public const string SignificantDigits = "SignificantDigits";

        /// <summary>
        /// The BrowseName for the SignificantDigitsValueDataType component.
        /// </summary>
        public const string SignificantDigitsValueDataType = "SignificantDigitsValueDataType";

        /// <summary>
        /// The BrowseName for the Sound_LevelConditionType component.
        /// </summary>
        public const string Sound_LevelConditionType = "Sound_LevelConditionType";

        /// <summary>
        /// The BrowseName for the SoundPressureType component.
        /// </summary>
        public const string SoundPressureType = "SoundPressureType";

        /// <summary>
        /// The BrowseName for the SourceType component.
        /// </summary>
        public const string SourceType = "SourceType";

        /// <summary>
        /// The BrowseName for the SpeedDataType component.
        /// </summary>
        public const string SpeedDataType = "SpeedDataType";

        /// <summary>
        /// The BrowseName for the Station component.
        /// </summary>
        public const string Station = "Station";

        /// <summary>
        /// The BrowseName for the StationDataType component.
        /// </summary>
        public const string StationDataType = "StationDataType";

        /// <summary>
        /// The BrowseName for the Status component.
        /// </summary>
        public const string Status = "Status";

        /// <summary>
        /// The BrowseName for the StrainConditionType component.
        /// </summary>
        public const string StrainConditionType = "StrainConditionType";

        /// <summary>
        /// The BrowseName for the StrainType component.
        /// </summary>
        public const string StrainType = "StrainType";

        /// <summary>
        /// The BrowseName for the SystemConditionType component.
        /// </summary>
        public const string SystemConditionType = "SystemConditionType";

        /// <summary>
        /// The BrowseName for the TemperatureConditionType component.
        /// </summary>
        public const string TemperatureConditionType = "TemperatureConditionType";

        /// <summary>
        /// The BrowseName for the TemperatureType component.
        /// </summary>
        public const string TemperatureType = "TemperatureType";

        /// <summary>
        /// The BrowseName for the TestIndicatorDataType component.
        /// </summary>
        public const string TestIndicatorDataType = "TestIndicatorDataType";

        /// <summary>
        /// The BrowseName for the TiltConditionType component.
        /// </summary>
        public const string TiltConditionType = "TiltConditionType";

        /// <summary>
        /// The BrowseName for the TiltType component.
        /// </summary>
        public const string TiltType = "TiltType";

        /// <summary>
        /// The BrowseName for the ToolAssetIdType component.
        /// </summary>
        public const string ToolAssetIdType = "ToolAssetIdType";

        /// <summary>
        /// The BrowseName for the ToolEventValueDataType component.
        /// </summary>
        public const string ToolEventValueDataType = "ToolEventValueDataType";

        /// <summary>
        /// The BrowseName for the ToolGroup component.
        /// </summary>
        public const string ToolGroup = "ToolGroup";

        /// <summary>
        /// The BrowseName for the ToolGroupDataType component.
        /// </summary>
        public const string ToolGroupDataType = "ToolGroupDataType";

        /// <summary>
        /// The BrowseName for the ToolId component.
        /// </summary>
        public const string ToolId = "ToolId";

        /// <summary>
        /// The BrowseName for the ToolIdDataType component.
        /// </summary>
        public const string ToolIdDataType = "ToolIdDataType";

        /// <summary>
        /// The BrowseName for the ToolLife component.
        /// </summary>
        public const string ToolLife = "ToolLife";

        /// <summary>
        /// The BrowseName for the ToolLifeDirectionTypeEnum component.
        /// </summary>
        public const string ToolLifeDirectionTypeEnum = "ToolLifeDirectionTypeEnum";

        /// <summary>
        /// The BrowseName for the ToolLifeTypeEnum component.
        /// </summary>
        public const string ToolLifeTypeEnum = "ToolLifeTypeEnum";

        /// <summary>
        /// The BrowseName for the ToolLifeValueDataType component.
        /// </summary>
        public const string ToolLifeValueDataType = "ToolLifeValueDataType";

        /// <summary>
        /// The BrowseName for the ToolNumberType component.
        /// </summary>
        public const string ToolNumberType = "ToolNumberType";

        /// <summary>
        /// The BrowseName for the TorqueConditionType component.
        /// </summary>
        public const string TorqueConditionType = "TorqueConditionType";

        /// <summary>
        /// The BrowseName for the TorqueType component.
        /// </summary>
        public const string TorqueType = "TorqueType";

        /// <summary>
        /// The BrowseName for the Type component.
        /// </summary>
        public const string Type = "Type";

        /// <summary>
        /// The BrowseName for the Units component.
        /// </summary>
        public const string Units = "Units";

        /// <summary>
        /// The BrowseName for the UnitsExtDataType component.
        /// </summary>
        public const string UnitsExtDataType = "UnitsExtDataType";

        /// <summary>
        /// The BrowseName for the VelocityConditionType component.
        /// </summary>
        public const string VelocityConditionType = "VelocityConditionType";

        /// <summary>
        /// The BrowseName for the VelocityType component.
        /// </summary>
        public const string VelocityType = "VelocityType";

        /// <summary>
        /// The BrowseName for the VersionDataType component.
        /// </summary>
        public const string VersionDataType = "VersionDataType";

        /// <summary>
        /// The BrowseName for the VibrationValueDataType component.
        /// </summary>
        public const string VibrationValueDataType = "VibrationValueDataType";

        /// <summary>
        /// The BrowseName for the ViscosityConditionType component.
        /// </summary>
        public const string ViscosityConditionType = "ViscosityConditionType";

        /// <summary>
        /// The BrowseName for the ViscosityType component.
        /// </summary>
        public const string ViscosityType = "ViscosityType";

        /// <summary>
        /// The BrowseName for the Volt_AmperageConditionType component.
        /// </summary>
        public const string Volt_AmperageConditionType = "Volt_AmperageConditionType";

        /// <summary>
        /// The BrowseName for the VoltageConditionType component.
        /// </summary>
        public const string VoltageConditionType = "VoltageConditionType";

        /// <summary>
        /// The BrowseName for the VoltageType component.
        /// </summary>
        public const string VoltageType = "VoltageType";

        /// <summary>
        /// The BrowseName for the VoltAmperageReactiveConditionType component.
        /// </summary>
        public const string VoltAmperageReactiveConditionType = "VoltAmperageReactiveConditionType";

        /// <summary>
        /// The BrowseName for the VoltAmpereReactiveType component.
        /// </summary>
        public const string VoltAmpereReactiveType = "VoltAmpereReactiveType";

        /// <summary>
        /// The BrowseName for the VoltAmpereType component.
        /// </summary>
        public const string VoltAmpereType = "VoltAmpereType";

        /// <summary>
        /// The BrowseName for the WarningLevel component.
        /// </summary>
        public const string WarningLevel = "WarningLevel";

        /// <summary>
        /// The BrowseName for the WattageConditionType component.
        /// </summary>
        public const string WattageConditionType = "WattageConditionType";

        /// <summary>
        /// The BrowseName for the WattageType component.
        /// </summary>
        public const string WattageType = "WattageType";

        /// <summary>
        /// The BrowseName for the WorkHoldingIdType component.
        /// </summary>
        public const string WorkHoldingIdType = "WorkHoldingIdType";
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
        /// The URI for the OpcUaMTConnect namespace (.NET code namespace is 'Opc.Ua.MTConnect').
        /// </summary>
        public const string OpcUaMTConnect = "http://opcfoundation.org/UA/MTConnect/";

        /// <summary>
        /// The URI for the OpcUaMTConnectXsd namespace (.NET code namespace is 'Opc.Ua.MTConnect').
        /// </summary>
        public const string OpcUaMTConnectXsd = "http://opcfoundation.org/UA/MTConnect/Types.xsd";

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
