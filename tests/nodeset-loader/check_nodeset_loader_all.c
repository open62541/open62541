/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer) */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "nodeset_loader_test.h"

#include <check.h>
#include <stdlib.h>
#include <string.h>

/* The order follows the RequiredModel dependencies. Older and newer revisions
 * of the same model are deliberately both covered. */
static const char *const nodesetFiles[] = {
    OPEN62541_NODESET_DIR "AMB/Opc.Ua.AMB.NodeSet2.xml",
    OPEN62541_NODESET_DIR "AML/Opc.Ua.AMLBaseTypes.NodeSet2.xml",
    OPEN62541_NODESET_DIR "AML/Opc.Ua.AMLLibraries.NodeSet2.xml",
    OPEN62541_NODESET_DIR "BACnet/Opc.Ua.BACnet.NodeSet2.xml",
    OPEN62541_NODESET_DIR "CNC/Opc.Ua.CNC.NodeSet.xml",
    OPEN62541_NODESET_DIR "DEXPI/Opc.Ua.DEXPI.NodeSet2.xml",
    OPEN62541_NODESET_DIR "DI/Opc.Ua.Di.NodeSet2.xml",
    OPEN62541_NODESET_DIR "ADI/Opc.Ua.Adi.NodeSet2.xml",
    OPEN62541_NODESET_DIR "AutoID/Opc.Ua.AutoID.NodeSet2.xml",
    OPEN62541_NODESET_DIR "CSPPlusForMachine/Opc.Ua.CSPPlusForMachine.NodeSet2.xml",
    OPEN62541_NODESET_DIR
    "CommercialKitchenEquipment/Opc.Ua.CommercialKitchenEquipment.NodeSet2.xml",
    OPEN62541_NODESET_DIR "DI/Opc.Ua.Di.PackageMetadata.NodeSet2.xml",
    OPEN62541_NODESET_DIR "DemoModel/DemoModel.NodeSet2.xml",
    OPEN62541_NODESET_DIR "FDI/Opc.Ua.Fdi5.NodeSet2.xml",
    OPEN62541_NODESET_DIR "FDI/Opc.Ua.Fdi7.NodeSet2.xml",
    OPEN62541_NODESET_DIR "FDT/Opc.Ua.FDT.NodeSet.xml",
    OPEN62541_NODESET_DIR "GDS/Opc.Ua.Gds.NodeSet2.xml",
    OPEN62541_NODESET_DIR "I4AAS/Opc.Ua.I4AAS.NodeSet2.xml",
    OPEN62541_NODESET_DIR "IA/Opc.Ua.IA.NodeSet2.xml",
    OPEN62541_NODESET_DIR "ECM/Opc.Ua.ECM.NodeSet2.xml",
    OPEN62541_NODESET_DIR "IA/Opc.Ua.IA.NodeSet2.examples.xml",
    OPEN62541_NODESET_DIR "IOLink/Opc.Ua.IOLink.NodeSet2.xml",
    OPEN62541_NODESET_DIR "IOLink/Opc.Ua.IOLinkIODD.NodeSet2.xml",
    OPEN62541_NODESET_DIR "IREDES/Opc.Ua.IREDES.NodeSet2.xml",
    OPEN62541_NODESET_DIR "ISA-95/Opc.ISA95.NodeSet2.xml",
    OPEN62541_NODESET_DIR "ISA95-JOBCONTROL/opc.ua.isa95-jobcontrol.nodeset2.xml",
    OPEN62541_NODESET_DIR "MDIS/Opc.MDIS.NodeSet2.xml",
    OPEN62541_NODESET_DIR "MTConnect/Opc.Ua.MTConnect.NodeSet2.xml",
    OPEN62541_NODESET_DIR "MachineVision/Opc.Ua.MachineVision.NodeSet2.xml",
    OPEN62541_NODESET_DIR "Machinery/Energy/Opc.Ua.Machinery.Energy.NodeSet2.xml",
    OPEN62541_NODESET_DIR "Machinery/Jobs/Opc.Ua.Machinery.Jobs.Nodeset2.xml",
    OPEN62541_NODESET_DIR "Machinery/Opc.Ua.Machinery.NodeSet2.xml",
    OPEN62541_NODESET_DIR "CAS/Opc.Ua.CAS.NodeSet2.xml",
    OPEN62541_NODESET_DIR "Glass/Flat/Opc.Ua.Glass.NodeSet2.xml",
    OPEN62541_NODESET_DIR "Glass/Flat/v2/Opc.Ua.Glass.v2.NodeSet2.xml",
    OPEN62541_NODESET_DIR "LADS/Opc.Ua.LADS.NodeSet2.xml",
    OPEN62541_NODESET_DIR "MachineTool/Opc.Ua.MachineTool.NodeSet2.xml",
    OPEN62541_NODESET_DIR "LaserSystems/Opc.Ua.LaserSystems.NodeSet2.xml",
    OPEN62541_NODESET_DIR "LaserSystems/LaserSystem-Example.NodeSet2.xml",
    OPEN62541_NODESET_DIR "MachineVision/AMCM/Opc.Ua.MachineVision.AMCM.NodeSet2.xml",
    OPEN62541_NODESET_DIR "Machinery/Result/Opc.Ua.Machinery_Result.NodeSet2.xml",
    OPEN62541_NODESET_DIR "GMS/opc.ua.gms.nodeset2.xml",
    OPEN62541_NODESET_DIR "CuttingTool/Opc.Ua.CuttingTool.NodeSet2.xml",
    OPEN62541_NODESET_DIR "IJT/Base/Opc.Ua.Ijt.Base.NodeSet2.xml",
    OPEN62541_NODESET_DIR "IJT/Tightening/Opc.Ua.Ijt.Tightening.NodeSet2.xml",
    OPEN62541_NODESET_DIR "Mining/DevelopmentSupport/General/1.0.0/"
                          "Opc.Ua.Mining.DevelopmentSupport.General.NodeSet2.xml",
    OPEN62541_NODESET_DIR "Mining/Extraction/General/1.0.0/"
                          "Opc.Ua.Mining.Extraction.General.NodeSet2.xml",
    OPEN62541_NODESET_DIR "Mining/General/1.0.0/Opc.Ua.Mining.General.NodeSet2.xml",
    OPEN62541_NODESET_DIR "Mining/DevelopmentSupport/Dozer/1.0.0/"
                          "Opc.Ua.Mining.DevelopmentSupport.Dozer.NodeSet2.xml",
    OPEN62541_NODESET_DIR "Mining/DevelopmentSupport/RoofSupportSystem/1.0.0/"
                          "Opc.Ua.Mining.DevelopmentSupport.RoofSupportSystem.NodeSet2.xml",
    OPEN62541_NODESET_DIR "Mining/Extraction/ShearerLoader/1.0.0/"
                          "Opc.Ua.Mining.Extraction.ShearerLoader.NodeSet2.xml",
    OPEN62541_NODESET_DIR "Mining/Loading/General/1.0.0/Opc.Ua.Mining.Loading.General.NodeSet2.xml",
    OPEN62541_NODESET_DIR "Mining/Loading/HydraulicExcavator/1.0.0/"
                          "Opc.Ua.Mining.Loading.HydraulicExcavator.NodeSet2.xml",
    OPEN62541_NODESET_DIR "Mining/MineralProcessing/General/1.0.0/"
                          "Opc.Ua.Mining.MineralProcessing.General.NodeSet2.xml",
    OPEN62541_NODESET_DIR "Mining/MineralProcessing/RockCrusher/1.0.0/"
                          "Opc.Ua.Mining.MineralProcessing.RockCrusher.NodeSet2.xml",
    OPEN62541_NODESET_DIR "Mining/MonitoringSupervisionServices/General/1.0.0/"
                          "Opc.Ua.Mining.MonitoringSupervisionServices.General.NodeSet2.xml",
    OPEN62541_NODESET_DIR "Mining/PELOServices/FaceAlignmentSystem/1.0.0/"
                          "Opc.Ua.Mining.PELOServices.FaceAlignmentSystem.NodeSet2.xml",
    OPEN62541_NODESET_DIR "Mining/PELOServices/General/1.0.0/"
                          "Opc.Ua.Mining.PELOServices.General.NodeSet2.xml",
    OPEN62541_NODESET_DIR "Mining/TransportDumping/ArmouredFaceConveyor/1.0.0/"
                          "Opc.Ua.Mining.TransportDumping.ArmouredFaceConveyor.NodeSet2.xml",
    OPEN62541_NODESET_DIR "Mining/TransportDumping/Conveying/"
                          "opc.ua.mining.transportdumping.conveying.nodeset2.xml",
    OPEN62541_NODESET_DIR "Mining/TransportDumping/General/1.0.0/"
                          "Opc.Ua.Mining.TransportDumping.General.NodeSet2.xml",
    OPEN62541_NODESET_DIR "Mining/TransportDumping/RearDumpTruck/1.0.0/"
                          "Opc.Ua.Mining.TransportDumping.RearDumpTruck.NodeSet2.xml",
    OPEN62541_NODESET_DIR "Onboarding/Opc.Ua.Onboarding.NodeSet2.xml",
    OPEN62541_NODESET_DIR "OpenSCS/Opc.Ua.OPENSCS.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PADIM/Opc.Ua.IRDI.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PLCopen/Opc.Ua.PLCopen.NodeSet2_V1.02.xml",
    OPEN62541_NODESET_DIR "PNEM/Opc.Ua.PnEm.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PNENC/Opc.Ua.PnEnc.Nodeset2.xml",
    OPEN62541_NODESET_DIR "PNDRV/Opc.Ua.PNDRV.Nodeset2.xml",
    OPEN62541_NODESET_DIR "PNGSDGM/opc.ua.pngsdgm.Nodeset2.xml",
    OPEN62541_NODESET_DIR "PNRIO/Opc.Ua.PnRio.Nodeset2.xml",
    OPEN62541_NODESET_DIR "POWERLINK/Opc.Ua.POWERLINK.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PROFINET/Opc.Ua.Pn.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PackML/Opc.Ua.PackML.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/GeneralTypes/1.02/"
                          "Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/GeneralTypes/1.03/"
                          "Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/Dosing/Opc.Ua.PlasticsRubber.Dosing.Nodeset2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/Extrusion/ExtrusionLine/1.00/"
                          "Opc.Ua.PlasticsRubber.Extrusion.ExtrusionLine.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/Extrusion/GeneralTypes/1.00/"
                          "Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/Extrusion/GeneralTypes/1.01/"
                          "Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/Extrusion/Calender/1.00/"
                          "Opc.Ua.PlasticsRubber.Extrusion.Calender.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/Extrusion/Calibrator/1.00/"
                          "Opc.Ua.PlasticsRubber.Extrusion.Calibrator.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/Extrusion/Cutter/1.00/"
                          "Opc.Ua.PlasticsRubber.Extrusion.Cutter.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/Extrusion/Die/1.00/"
                          "Opc.Ua.PlasticsRubber.Extrusion.Die.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/Extrusion/Extruder/1.00/"
                          "Opc.Ua.PlasticsRubber.Extrusion.Extruder.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/Extrusion/Filter/1.00/"
                          "Opc.Ua.PlasticsRubber.Extrusion.Filter.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/Extrusion/HaulOff/1.00/"
                          "Opc.Ua.PlasticsRubber.Extrusion.HaulOff.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/Extrusion/Corrugator/1.00/"
                          "Opc.Ua.PlasticsRubber.Extrusion.Corrugator.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/Extrusion/MeltPump/1.00/"
                          "Opc.Ua.PlasticsRubber.Extrusion.MeltPump.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/Extrusion/Pelletizer/1.00/"
                          "Opc.Ua.PlasticsRubber.Extrusion.Pelletizer.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/"
                          "Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/Extrusion_v2/Calender/2.00/"
                          "Opc.Ua.PlasticsRubber.Extrusion_v2.Calender.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/Extrusion_v2/Calibrator/2.00/"
                          "Opc.Ua.PlasticsRubber.Extrusion_v2.Calibrator.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/Extrusion_v2/Cutter/2.00/"
                          "Opc.Ua.PlasticsRubber.Extrusion_v2.Cutter.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/Extrusion_v2/Die/2.00/"
                          "Opc.Ua.PlasticsRubber.Extrusion_v2.Die.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/Extrusion_v2/Extruder/2.00/"
                          "Opc.Ua.PlasticsRubber.Extrusion_v2.Extruder.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/Extrusion_v2/ExtrusionLine/2.00/"
                          "Opc.Ua.PlasticsRubber.Extrusion_v2.ExtrusionLine.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/Extrusion_v2/Filter/2.00/"
                          "Opc.Ua.PlasticsRubber.Extrusion_v2.Filter.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/Extrusion_v2/HaulOff/2.00/"
                          "Opc.Ua.PlasticsRubber.Extrusion_v2.HaulOff.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/Extrusion_v2/Corrugator/2.00/"
                          "Opc.Ua.PlasticsRubber.Extrusion_v2.Corrugator.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/Extrusion_v2/MeltPump/2.00/"
                          "Opc.Ua.PlasticsRubber.Extrusion_v2.MeltPump.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/Extrusion_v2/Pelletizer/2.00/"
                          "Opc.Ua.PlasticsRubber.Extrusion_v2.Pelletizer.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/FlexiblePURFoam/Cutting/"
                          "Opc.Ua.PlasticsRubber.FoamCutting.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/GDD/Opc.Ua.PlasticsRubber.GDD.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/HotRunner/1.00/"
                          "Opc.Ua.PlasticsRubber.HotRunner.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/IMM2MES/1.01/Opc.Ua.PlasticsRubber.IMM2MES.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/LDS/1.00/Opc.Ua.PlasticsRubber.LDS.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/LDS/1.02/Opc.Ua.PlasticsRubber.LDS.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PlasticsRubber/TCD/1.01/Opc.Ua.PlasticsRubber.TCD.NodeSet2.xml",
    OPEN62541_NODESET_DIR "Powertrain/powertraindictionary.nodeset2.xml",
    OPEN62541_NODESET_DIR "PADIM/Opc.Ua.PADIM.NodeSet2.xml",
    OPEN62541_NODESET_DIR "Machinery/ProcessValues/Opc.Ua.Machinery.ProcessValues.NodeSet2.xml",
    OPEN62541_NODESET_DIR "AdditiveManufacturing/Opc.Ua.AdditiveManufacturing.Nodeset2.xml",
    OPEN62541_NODESET_DIR "Machinery/Opc.Ua.Machinery.Examples.NodeSet2.xml",
    OPEN62541_NODESET_DIR "MetalForming/Opc.Ua.MetalForming.NodeSet2.xml",
    OPEN62541_NODESET_DIR "PAEFS/Opc.Ua.PAEFS.NodeSet2.xml",
    OPEN62541_NODESET_DIR "Pumps/Opc.Ua.Pumps.NodeSet2.xml",
    OPEN62541_NODESET_DIR "RSL/Opc.Ua.RSL.NodeSet2.xml",
    OPEN62541_NODESET_DIR "GPOS/Opc.Ua.GPOS.NodeSet2.xml",
    OPEN62541_NODESET_DIR "Robotics/Opc.Ua.Robotics.NodeSet2.xml",
    OPEN62541_NODESET_DIR "CranesHoists/Opc.Ua.CranesHoists.NodeSet2.xml",
    OPEN62541_NODESET_DIR "Safety/Opc.Ua.Safety.NodeSet2.xml",
    OPEN62541_NODESET_DIR "Scales/Opc.Ua.Scales.NodeSet2.xml",
    OPEN62541_NODESET_DIR "Scheduler/Opc.Ua.Scheduler.NodeSet2.xml",
    OPEN62541_NODESET_DIR "Sercos/Sercos.NodeSet2.xml",
    OPEN62541_NODESET_DIR "Shotblasting/Opc.Ua.Shotblasting.NodeSet2.xml",
    OPEN62541_NODESET_DIR "Shotblasting/Opc.Ua.Shotblasting.NodeSet2.profiles.xml",
    OPEN62541_NODESET_DIR "SurfaceTechnology/GeneralTypes/Opc.Ua.STGeneralTypes.NodeSet2.xml",
    OPEN62541_NODESET_DIR "SurfaceTechnology/CleaningPretreatment/"
                          "Opc.Ua.ST.CleaningPretreatment.NodeSet2.xml",
    OPEN62541_NODESET_DIR "SurfaceTechnology/OCT-MSS/Opc.Ua.ST-Mss.NodeSet2.xml",
    OPEN62541_NODESET_DIR "SurfaceTechnology/Plasma/Opc.Ua.SurfaceTechnology.Plasma.NodeSet2.xml",
    OPEN62541_NODESET_DIR "TMC/Opc.Ua.TMC.NodeSet2.xml",
    OPEN62541_NODESET_DIR "TTD/opc.ua.ttd.nodeset2.xml",
    OPEN62541_NODESET_DIR "TestModel/TestModel.NodeSet2.xml",
    OPEN62541_NODESET_DIR "UAFX/opc.ua.fx.data.nodeset2.xml",
    OPEN62541_NODESET_DIR "UAFX/opc.ua.fx.ac.nodeset2.xml",
    OPEN62541_NODESET_DIR "Powertrain/Opc.Ua.Powertrain.NodeSet2.xml",
    OPEN62541_NODESET_DIR "UAFX/opc.ua.fx.cm.nodeset2.xml",
    OPEN62541_NODESET_DIR "UnattendedRetail/Opc.Ua.UnattendedRetail.NodeSet2.xml",
    OPEN62541_NODESET_DIR "WMTP/Opc.Ua.WMTP.Nodeset2.xml",
    OPEN62541_NODESET_DIR "WMTP/Opc.Ua.WMTP.NodeSet2.profiles.xml",
    OPEN62541_NODESET_DIR "Weihenstephan/Opc.Ua.Weihenstephan.NodeSet2.xml",
    OPEN62541_NODESET_DIR "WireHarness/opc.ua.wireharness.vec.nodeset2.xml",
    OPEN62541_NODESET_DIR "WireHarness/opc.ua.wireharness.nodeset2.xml",
    OPEN62541_NODESET_DIR "WoT/Opc.Ua.WotCon.NodeSet2.xml",
    OPEN62541_NODESET_DIR "Woodworking/Opc.Ua.Woodworking.NodeSet2.xml",
    OPEN62541_NODESET_DIR "Woodworking/Opc.Ua.Eumabois.Nodeset2.xml",
    OPEN62541_NODESET_DIR "XML/Opc.Ua.Xml.NodeSet2.xml",
};

static const size_t nodesetFilesSize = sizeof(nodesetFiles) / sizeof(nodesetFiles[0]);

static const UA_UInt32 bacnetDataTypeIds[] = {
    3046, 103002, 103039, 103040, 103032, 103034, 3068, 103031, 3050, 3051, 103017,
};

static const UA_UInt32 amlInheritedValueRankIds[] = {
    2, 3, 5, 6, 8, 9, 11, 12, 14, 15,
};

static const UA_UInt32 amlArgumentDescriptorIds[] = {
    46, 49, 67, 104, 113, 116, 119,
};

static const UA_UInt32 bacnetEnumValueIds[] = {
    6007,
    6056,
    6360,
};

static const UA_UInt32 powerlinkInheritedDataTypeIds[] = {
    14, 1491, 1506, 1521, 1536, 1551, 1566, 2152, 2182,
};

static const UA_UInt32 i4aasValueIds[] = {
    6003, 6002, 6004, 6005, 6112, 6113, 6076, 6077, 6079,
};

static UA_Server *
newServer(void) {
    UA_ServerConfig config;
    memset(&config, 0, sizeof(config));
    config.logging = UA_Log_Stdout_new(UA_LOGLEVEL_WARNING);
    if(!config.logging)
        return NULL;

    UA_StatusCode res = UA_ServerConfig_setMinimal(&config, 0, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_clear(&config);
        return NULL;
    }
    return UA_Server_newWithConfig(&config);
}

static void
assertNodeExists(UA_Server *server, const char *namespaceUri, UA_UInt32 numericId) {
    size_t namespaceIndex;
    UA_StatusCode res = UA_Server_getNamespaceByName(
        server, UA_STRING((char *)(uintptr_t)namespaceUri), &namespaceIndex);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(namespaceIndex <= UA_UINT16_MAX);

    UA_NodeClass nodeClass;
    UA_NodeId id = UA_NODEID_NUMERIC((UA_UInt16)namespaceIndex, numericId);
    res = UA_Server_readNodeClass(server, id, &nodeClass);
    ck_assert_msg(res == UA_STATUSCODE_GOOD, "Node %u in %s was not added", (unsigned)numericId,
                  namespaceUri);
}

static UA_NodeId
nodeIdForNamespace(UA_Server *server, const char *namespaceUri, UA_UInt32 numericId) {
    size_t namespaceIndex;
    UA_StatusCode res = UA_Server_getNamespaceByName(
        server, UA_STRING((char *)(uintptr_t)namespaceUri), &namespaceIndex);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(namespaceIndex <= UA_UINT16_MAX);
    return UA_NODEID_NUMERIC((UA_UInt16)namespaceIndex, numericId);
}

static void
assertArrayDimensions(UA_Server *server, const char *namespaceUri, UA_UInt32 numericId,
                      size_t expectedSize, const UA_UInt32 *expected) {
    UA_NodeId id = nodeIdForNamespace(server, namespaceUri, numericId);
    UA_Variant dimensions;
    UA_Variant_init(&dimensions);
    UA_StatusCode res = UA_Server_readArrayDimensions(server, id, &dimensions);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasArrayType(&dimensions, &UA_TYPES[UA_TYPES_UINT32]));
    ck_assert_uint_eq(dimensions.arrayLength, expectedSize);
    for(size_t i = 0; i < expectedSize; i++)
        ck_assert_uint_eq(((UA_UInt32 *)dimensions.data)[i], expected[i]);
    UA_Variant_clear(&dimensions);
}

static void
assertUInt32ArrayValue(UA_Server *server, const char *namespaceUri, UA_UInt32 numericId) {
    UA_NodeId id = nodeIdForNamespace(server, namespaceUri, numericId);
    UA_Variant value;
    UA_Variant_init(&value);
    UA_StatusCode res = UA_Server_readValue(server, id, &value);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(value.type, &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_uint_eq(value.arrayLength, 1);
    ck_assert_uint_eq(*(UA_UInt32 *)value.data, 0);
    UA_Variant_clear(&value);
}

START_TEST(loadAllNodesetsIntoOneServer) {
    UA_Server *server = newServer();
    ck_assert_ptr_nonnull(server);

    UA_StatusCode res = UA_Server_run_startup(server);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    for(size_t i = 0; i < nodesetFilesSize; i++) {
        fprintf(stderr, "Loading NodeSet %lu/%lu: %s\n", (unsigned long)(i + 1),
                (unsigned long)nodesetFilesSize, nodesetFiles[i]);
        res = loadNodesetFile(server, nodesetFiles[i], NULL);
        ck_assert_msg(res == UA_STATUSCODE_GOOD, "Loading %s failed with %s", nodesetFiles[i],
                      UA_StatusCode_name(res));
    }

    size_t bacnetNamespace;
    res = UA_Server_getNamespaceByName(server, UA_STRING("http://opcfoundation.org/UA/BACnet_V2/"),
                                       &bacnetNamespace);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(bacnetNamespace <= UA_UINT16_MAX);
    for(size_t i = 0; i < sizeof(bacnetDataTypeIds) / sizeof(bacnetDataTypeIds[0]); i++) {
        UA_NodeId id = UA_NODEID_NUMERIC((UA_UInt16)bacnetNamespace, bacnetDataTypeIds[i]);
        const UA_DataType *type = UA_Server_findDataType(server, &id);
        ck_assert_msg(type != NULL, "BACnet DataType %u was not registered",
                      (unsigned)bacnetDataTypeIds[i]);
        if(bacnetDataTypeIds[i] == 3046) {
            ck_assert_uint_eq(type->typeKind, UA_DATATYPEKIND_ENUM);
            ck_assert_uint_eq(type->membersSize, 387);
        }
    }

    assertNodeExists(server, "http://opcfoundation.org/UA/MachineVision/AMCM/", 5006);
    assertNodeExists(server, "http://opcfoundation.org/UA/RSL/", 6038);

    size_t amlNamespace;
    res = UA_Server_getNamespaceByName(server, UA_STRING("http://opcfoundation.org/UA/AMLLibs/"),
                                       &amlNamespace);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(amlNamespace <= UA_UINT16_MAX);
    for(size_t i = 0; i < sizeof(amlInheritedValueRankIds) / sizeof(amlInheritedValueRankIds[0]);
        i++) {
        UA_NodeId id = UA_NODEID_NUMERIC((UA_UInt16)amlNamespace, amlInheritedValueRankIds[i]);
        UA_Int32 valueRank;
        res = UA_Server_readValueRank(server, id, &valueRank);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
        ck_assert_int_eq(valueRank, UA_VALUERANK_ANY);

        UA_Variant value;
        UA_Variant_init(&value);
        res = UA_Server_readValue(server, id, &value);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(value.arrayLength, 1);
        UA_Variant_clear(&value);
    }

    for(size_t i = 0; i < sizeof(amlArgumentDescriptorIds) / sizeof(amlArgumentDescriptorIds[0]);
        i++) {
        UA_NodeId id = UA_NODEID_NUMERIC((UA_UInt16)amlNamespace, amlArgumentDescriptorIds[i]);
        UA_Variant value;
        UA_Variant_init(&value);
        res = UA_Server_readValue(server, id, &value);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
        ck_assert(UA_Variant_isScalar(&value));
        const UA_DataType *expectedType = &UA_TYPES[UA_TYPES_STRING];
        if(amlArgumentDescriptorIds[i] == 67)
            expectedType = &UA_TYPES[UA_TYPES_BOOLEAN];
        ck_assert_ptr_eq(value.type, expectedType);
        UA_Variant_clear(&value);
    }

    for(size_t i = 0; i < sizeof(bacnetEnumValueIds) / sizeof(bacnetEnumValueIds[0]); i++) {
        UA_NodeId id = UA_NODEID_NUMERIC((UA_UInt16)bacnetNamespace, bacnetEnumValueIds[i]);
        UA_Variant value;
        UA_Variant_init(&value);
        res = UA_Server_readValue(server, id, &value);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
        ck_assert(UA_Variant_isScalar(&value));
        UA_NodeId enumTypeId = UA_NODEID_NUMERIC((UA_UInt16)bacnetNamespace, 3003);
        ck_assert(UA_NodeId_equal(&value.type->typeId, &enumTypeId));
        ck_assert_int_eq(*(UA_Int32 *)value.data, 0);
        UA_Variant_clear(&value);
    }

    size_t powerlinkNamespace;
    res = UA_Server_getNamespaceByName(server, UA_STRING("http://opcfoundation.org/UA/POWERLINK/"),
                                       &powerlinkNamespace);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(powerlinkNamespace <= UA_UINT16_MAX);
    for(size_t i = 0;
        i < sizeof(powerlinkInheritedDataTypeIds) / sizeof(powerlinkInheritedDataTypeIds[0]); i++) {
        UA_NodeId id =
            UA_NODEID_NUMERIC((UA_UInt16)powerlinkNamespace, powerlinkInheritedDataTypeIds[i]);
        UA_NodeId dataType;
        UA_NodeId_init(&dataType);
        res = UA_Server_readDataType(server, id, &dataType);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
        ck_assert(UA_NodeId_equal(&dataType, &UA_TYPES[UA_TYPES_BYTE].typeId));
        UA_NodeId_clear(&dataType);
    }

    static const UA_UInt32 expectedIOLinkDimensions[] = {0, 0};
    assertArrayDimensions(server, "http://opcfoundation.org/UA/IOLink/", 6146, 2,
                          expectedIOLinkDimensions);

    UA_NodeId i4aasType = nodeIdForNamespace(server, "http://opcfoundation.org/UA/I4AAS/", 3011);
    for(size_t i = 0; i < sizeof(i4aasValueIds) / sizeof(i4aasValueIds[0]); i++) {
        UA_NodeId id =
            nodeIdForNamespace(server, "http://opcfoundation.org/UA/I4AAS/", i4aasValueIds[i]);
        UA_Variant value;
        UA_Variant_init(&value);
        res = UA_Server_readValue(server, id, &value);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(value.arrayLength, 1);
        ck_assert_ptr_nonnull(value.type);
        ck_assert(UA_NodeId_equal(&value.type->typeId, &i4aasType));
        UA_Variant_clear(&value);
    }

    assertUInt32ArrayValue(server, "http://opcfoundation.org/UA/PADIM/", 6016);
    assertUInt32ArrayValue(server, "http://opcfoundation.org/UA/PADIM/", 6031);
    static const UA_UInt32 expected20[] = {20};
    static const UA_UInt32 expected6[] = {6};
    assertArrayDimensions(server, "http://opcfoundation.org/UA/FX/AC/", 6084, 1, expected20);
    assertArrayDimensions(server, "http://opcfoundation.org/UA/FX/AC/", 6162, 1, expected6);
    assertArrayDimensions(server, "http://opcfoundation.org/UA/FX/CM/", 6244, 1, expected6);

    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}
END_TEST

int
main(void) {
    Suite *suite = suite_create("All Nodesets");
    TCase *tc = tcase_create("Load");
    tcase_set_timeout(tc, 1800);
    tcase_add_test(tc, loadAllNodesetsIntoOneServer);
    suite_add_tcase(suite, tc);

    SRunner *runner = srunner_create(suite);
    srunner_set_fork_status(runner, CK_NOFORK);
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
