/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Test case for all standardized companion nodesets. You can
 * find the specifications at https://github.com/OPCFoundation/UA-Nodeset.
 *      - branch: latest
 *      - commit: 54e3513
 * 
 * Currently this test case is missing the following UA-Nodesets:
 *    * NodesetLoader related issues:
 *      - ${OPEN62541_NODESET_DIR}AML/Opc.Ua.AMLLibraries.NodeSet2.xml
 *      - ${OPEN62541_NODESET_DIR}ISA95-JOBCONTROL/opc.ua.isa95-jobcontrol.nodeset2.xml
 *      - ${OPEN62541_NODESET_DIR}TMC/Opc.Ua.TMC.NodeSet2.xml
 *
 *    * Memory leak issues (libXML2 library related):
 *      - ${OPEN62541_NODESET_DIR}CNC/Opc.Ua.CNC.NodeSet.xml
 *      - ${OPEN62541_NODESET_DIR}IJT/Tightening/Opc.Ua.Ijt.Tightening.NodeSet2.xml
 *      - $(OPEN62541_NODESET_DIR)MachineVision/Opc.Ua.MachineVision.NodeSet2.xml
 *      - $(OPEN62541_NODESET_DIR)Mining/General/1.0.0/Opc.Ua.Mining.General.NodeSet2.xml
 *          - Dependent Nodesets:
 *              * $(OPEN62541_NODESET_DIR)Mining/DevelopmentSupport/Dozer/1.0.0/Opc.Ua.Mining.DevelopmentSupport.Dozer.NodeSet2.xml
 *              * $(OPEN62541_NODESET_DIR)Mining/DevelopmentSupport/RoofSupportSystem/1.0.0/Opc.Ua.Mining.DevelopmentSupport.RoofSupportSystem.NodeSet2.xml
 *              * $(OPEN62541_NODESET_DIR)Mining/Extraction/ShearerLoader/1.0.0/Opc.Ua.Mining.Extraction.ShearerLoader.NodeSet2.xml
 *              * $(OPEN62541_NODESET_DIR)Mining/Loading/General/1.0.0/Opc.Ua.Mining.Loading.General.NodeSet2.xml
 *              * $(OPEN62541_NODESET_DIR)Mining/Loading/HydraulicExcavator/1.0.0/Opc.Ua.Mining.Loading.HydraulicExcavator.NodeSet2.xml
 *              * $(OPEN62541_NODESET_DIR)Mining/MineralProcessing/RockCrusher/1.0.0/Opc.Ua.Mining.MineralProcessing.RockCrusher.NodeSet2.xml
 *              * $(OPEN62541_NODESET_DIR)Mining/PELOServices/FaceAlignmentSystem/1.0.0/Opc.Ua.Mining.PELOServices.FaceAlignmentSystem.NodeSet2.xml
 *              * $(OPEN62541_NODESET_DIR)Mining/PELOServices/General/1.0.0/Opc.Ua.Mining.PELOServices.General.NodeSet2.xml
 *              * $(OPEN62541_NODESET_DIR)Mining/TransportDumping/ArmouredFaceConveyor/1.0.0/Opc.Ua.Mining.TransportDumping.ArmouredFaceConveyor.NodeSet2.xml
 *              * $(OPEN62541_NODESET_DIR)Mining/TransportDumping/General/1.0.0/Opc.Ua.Mining.TransportDumping.General.NodeSet2.xml
 *              * $(OPEN62541_NODESET_DIR)Mining/TransportDumping/RearDumpTruck/1.0.0/Opc.Ua.Mining.TransportDumping.RearDumpTruck.NodeSet2.xml
 *      - $(OPEN62541_NODESET_DIR)PADIM/Opc.Ua.IRDI.NodeSet2.xml
 *          - Dependent Nodesets:
 *              * $(OPEN62541_NODESET_DIR)PADIM/Opc.Ua.PADIM.NodeSet2.xml
 *      - $(OPEN62541_NODESET_DIR)POWERLINK/Opc.Ua.POWERLINK.NodeSet2.xml
 *      - $(OPEN62541_NODESET_DIR)Pumps/Opc.Ua.Pumps.NodeSet2.xml
 *      - $(OPEN62541_NODESET_DIR)Scales/Opc.Ua.Scales.NodeSet2.xml
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/nodesetloader.h>

#include "check.h"
#include "testing_clock.h"
#include "test_helpers.h"

UA_Server *server = NULL;

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_Server_run_startup(server);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(Server_loadADINodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "ADI/Opc.Ua.Adi.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadAMBNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "AMB/Opc.Ua.AMB.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadAMLBaseTypesNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "AML/Opc.Ua.AMLBaseTypes.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadAutoIDNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "AutoID/Opc.Ua.AutoID.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadBACnetNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "BACnet/Opc.Ua.BACnet.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadCASNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "CAS/Opc.Ua.CAS.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadCommercialKitchenEquipmentNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "CommercialKitchenEquipment/Opc.Ua.CommercialKitchenEquipment.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadCSPPlusForMachineNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "CSPPlusForMachine/Opc.Ua.CSPPlusForMachine.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadDEXPINodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "DEXPI/Opc.Ua.DEXPI.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadDINodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "DI/Opc.Ua.Di.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadDotNetNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "DotNet/Opc.Ua.NodeSet.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadFDI5Nodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "FDI/Opc.Ua.Fdi5.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadFDI7Nodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "FDI/Opc.Ua.Fdi7.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadFDTNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "FDT/Opc.Ua.FDT.NodeSet.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadGDSNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "GDS/Opc.Ua.Gds.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST


START_TEST(Server_loadServer_loadGlassNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "Glass/Flat/Opc.Ua.Glass.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadI4AASNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "I4AAS/Opc.Ua.I4AAS.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadIANodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "IA/Opc.Ua.IA.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadIAExamplesNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "IA/Opc.Ua.IA.NodeSet2.examples.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadIOLinkIODDNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "IOLink/Opc.Ua.IOLinkIODD.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadIOLinkNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "IOLink/Opc.Ua.IOLink.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadISA95Nodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "ISA-95/Opc.ISA95.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadMachineryNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "Machinery/Opc.Ua.Machinery.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadMachineryExamplesNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "Machinery/Opc.Ua.Machinery.Examples.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadMachineToolNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "MachineTool/Opc.Ua.MachineTool.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

// START_TEST(Server_loadMDISNodeset) {
//     UA_StatusCode retVal = UA_Server_loadNodeset(server,
//         OPEN62541_NODESET_DIR "MDIS/Opc.MDIS.NodeSet2.xml", NULL);
//     ck_assert(UA_StatusCode_isGood(retVal));
// }
// END_TEST

START_TEST(Server_loadMiningDevelopmentSupportGeneralNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "Mining/DevelopmentSupport/General/1.0.0/Opc.Ua.Mining.DevelopmentSupport.General.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadMiningExtractionGeneralNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "Mining/Extraction/General/1.0.0/Opc.Ua.Mining.Extraction.General.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadMiningMineralProcessingGeneralNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "Mining/MineralProcessing/General/1.0.0/Opc.Ua.Mining.MineralProcessing.General.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadMiningMonitoringSupervisionServicesGeneralNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "Mining/MonitoringSupervisionServices/General/1.0.0/Opc.Ua.Mining.MonitoringSupervisionServices.General.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadMTConnectNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "MTConnect/Opc.Ua.MTConnect.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadOPENSCSNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "OpenSCS/Opc.Ua.OPENSCS.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPackMLNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "PackML/Opc.Ua.PackML.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPlasticsRubberExtrusionCalenderNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "PlasticsRubber/Extrusion/Calender/1.00/Opc.Ua.PlasticsRubber.Extrusion.Calender.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPlasticsRubberExtrusionCalibratorNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "PlasticsRubber/Extrusion/Calibrator/1.00/Opc.Ua.PlasticsRubber.Extrusion.Calibrator.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPlasticsRubberExtrusionCorrugatorNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "PlasticsRubber/Extrusion/Corrugator/1.00/Opc.Ua.PlasticsRubber.Extrusion.Corrugator.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPlasticsRubberExtrusionCutterNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "PlasticsRubber/Extrusion/Cutter/1.00/Opc.Ua.PlasticsRubber.Extrusion.Cutter.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPlasticsRubberExtrusionDieNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "PlasticsRubber/Extrusion/Die/1.00/Opc.Ua.PlasticsRubber.Extrusion.Die.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPlasticsRubberExtrusionExtruderNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "PlasticsRubber/Extrusion/Extruder/1.00/Opc.Ua.PlasticsRubber.Extrusion.Extruder.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPlasticsRubberExtrusionExtrusionLineNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "PlasticsRubber/Extrusion/ExtrusionLine/1.00/Opc.Ua.PlasticsRubber.Extrusion.ExtrusionLine.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPlasticsRubberExtrusionFilterNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "PlasticsRubber/Extrusion/Filter/1.00/Opc.Ua.PlasticsRubber.Extrusion.Filter.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPlasticsRubberExtrusionGeneralTypes_v1_0_0_Nodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "PlasticsRubber/Extrusion/GeneralTypes/1.00/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPlasticsRubberExtrusionGeneralTypes_v1_0_1_Nodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "PlasticsRubber/Extrusion/GeneralTypes/1.01/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPlasticsRubberExtrusionHaulOffNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "PlasticsRubber/Extrusion/HaulOff/1.00/Opc.Ua.PlasticsRubber.Extrusion.HaulOff.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPlasticsRubberExtrusionMeltPumpNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "PlasticsRubber/Extrusion/MeltPump/1.00/Opc.Ua.PlasticsRubber.Extrusion.MeltPump.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPlasticsRubberExtrusionPelletizerNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "PlasticsRubber/Extrusion/Pelletizer/1.00/Opc.Ua.PlasticsRubber.Extrusion.Pelletizer.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPlasticsRubberExtrusionv2CalenderNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "PlasticsRubber/Extrusion_v2/Calender/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Calender.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPlasticsRubberExtrusionv2CalibratorNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "PlasticsRubber/Extrusion_v2/Calibrator/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Calibrator.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPlasticsRubberExtrusionv2CorrugatorNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "PlasticsRubber/Extrusion_v2/Corrugator/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Corrugator.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPlasticsRubberExtrusionv2CutterNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "PlasticsRubber/Extrusion_v2/Cutter/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Cutter.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPlasticsRubberExtrusionv2DieNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "PlasticsRubber/Extrusion_v2/Die/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Die.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPlasticsRubberExtrusionv2ExtruderNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "PlasticsRubber/Extrusion_v2/Extruder/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Extruder.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPlasticsRubberExtrusionv2ExtrusionLineNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "PlasticsRubber/Extrusion_v2/ExtrusionLine/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.ExtrusionLine.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPlasticsRubberExtrusionv2FilterNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "PlasticsRubber/Extrusion_v2/Filter/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Filter.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPlasticsRubberExtrusionv2GeneralTypesNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPlasticsRubberExtrusionv2HaulOffNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "PlasticsRubber/Extrusion_v2/HaulOff/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.HaulOff.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPlasticsRubberExtrusionv2MeltPumpNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "PlasticsRubber/Extrusion_v2/MeltPump/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.MeltPump.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPlasticsRubberExtrusionv2PelletizerNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "PlasticsRubber/Extrusion_v2/Pelletizer/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Pelletizer.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPlasticsRubberGeneralTypes_v1_0_2_Nodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPlasticsRubberGeneralTypes_v1_0_3_Nodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPlasticsRubberHotRunnerNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "PlasticsRubber/HotRunner/1.00/Opc.Ua.PlasticsRubber.HotRunner.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPlasticsRubberIMM2MESNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "PlasticsRubber/IMM2MES/1.01/Opc.Ua.PlasticsRubber.IMM2MES.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPlasticsRubberLDSNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "PlasticsRubber/LDS/1.00/Opc.Ua.PlasticsRubber.LDS.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPlasticsRubberTCDNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR
        "PlasticsRubber/TCD/1.01/Opc.Ua.PlasticsRubber.TCD.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPLCopenNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "PLCopen/Opc.Ua.PLCopen.NodeSet2_V1.02.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPnEmNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "PNEM/Opc.Ua.PnEm.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPnRioNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "PNRIO/Opc.Ua.PnRio.Nodeset2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadPROFINETNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "PROFINET/Opc.Ua.Pn.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadRoboticsNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "Robotics/Opc.Ua.Robotics.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadSafetyNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "Safety/Opc.Ua.Safety.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadSercosNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "Sercos/Sercos.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadWeihenstephanNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "Weihenstephan/Opc.Ua.Weihenstephan.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadWoodworkingEumaboisNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "Woodworking/Opc.Ua.Eumabois.Nodeset2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadWoodworkingNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "Woodworking/Opc.Ua.Woodworking.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Server Nodeset Loader");
    {
        TCase *tc_server = tcase_create("Server load ADI nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadADINodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load AMB nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadAMBNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load AMLBaseTypes nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadAMLBaseTypesNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load AutoID nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadAutoIDNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load BACnet nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadBACnetNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load CAS nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadIANodeset);
        tcase_add_test(tc_server, Server_loadMachineryNodeset);
        tcase_add_test(tc_server, Server_loadCASNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load CommercialKitchenEquipment nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadCommercialKitchenEquipmentNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load CSPPlusForMachine nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadCSPPlusForMachineNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load DEXPI nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDEXPINodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load DI nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load DotNet nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDotNetNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load FDI5 nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadFDI5Nodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load FDI7 nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadFDI7Nodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load FDT nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadFDTNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load GDS nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadGDSNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load Glass nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadMachineryNodeset);
        tcase_add_test(tc_server, Server_loadServer_loadGlassNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load I4AAS nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadI4AASNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load IA nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadIANodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load IAExamples nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadIANodeset);
        tcase_add_test(tc_server, Server_loadIAExamplesNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load IOLinkIODD nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadIOLinkIODDNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load IOLink nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadIOLinkNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load ISA95 nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadISA95Nodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load Machinery nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadMachineryNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load MachineryExamples nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadMachineryNodeset);
        tcase_add_test(tc_server, Server_loadMachineryExamplesNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load MachineTool nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadMachineryNodeset);
        tcase_add_test(tc_server, Server_loadIANodeset);
        tcase_add_test(tc_server, Server_loadMachineToolNodeset);
        suite_add_tcase(s, tc_server);
    }
    // {
    //     TCase *tc_server = tcase_create("Server load MDIS nodeset");
    //     tcase_add_unchecked_fixture(tc_server, setup, teardown);
    //     tcase_add_test(tc_server, Server_loadMDISNodeset);
    //     suite_add_tcase(s, tc_server);
    // }
    {
        TCase *tc_server = tcase_create("Server load MiningDevelopmentSupportGeneral nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadMiningDevelopmentSupportGeneralNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load MiningExtractionGeneral nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadMiningExtractionGeneralNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load MiningMineralProcessingGeneral nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadMiningMineralProcessingGeneralNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load MiningMonitoringSupervisionServicesGeneral nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadMiningMonitoringSupervisionServicesGeneralNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load MTConnect nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadMTConnectNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load OPENSCS nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadOPENSCSNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PackML nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadPackMLNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PlasticsRubberExtrusionCalender nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberGeneralTypes_v1_0_2_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionGeneralTypes_v1_0_1_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionCalenderNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PlasticsRubberExtrusionCalibrator nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberGeneralTypes_v1_0_2_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionGeneralTypes_v1_0_0_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionCalibratorNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PlasticsRubberExtrusionCorrugator nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberGeneralTypes_v1_0_2_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionGeneralTypes_v1_0_0_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionHaulOffNodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionCorrugatorNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PlasticsRubberExtrusionCutter nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberGeneralTypes_v1_0_2_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionGeneralTypes_v1_0_0_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionCutterNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PlasticsRubberExtrusionDie nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberGeneralTypes_v1_0_2_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionGeneralTypes_v1_0_0_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionDieNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PlasticsRubberExtrusionExtruder nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberGeneralTypes_v1_0_2_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionGeneralTypes_v1_0_0_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionExtruderNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PlasticsRubberExtrusionExtrusionLine nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberGeneralTypes_v1_0_2_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionExtrusionLineNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PlasticsRubberExtrusionFilter nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberGeneralTypes_v1_0_2_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionGeneralTypes_v1_0_0_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionFilterNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PlasticsRubberExtrusionGeneralTypes v1.00 nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberGeneralTypes_v1_0_2_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionGeneralTypes_v1_0_0_Nodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PlasticsRubberExtrusionGeneralTypes v1.01 nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberGeneralTypes_v1_0_2_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionGeneralTypes_v1_0_1_Nodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PlasticsRubberExtrusionHaulOff nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberGeneralTypes_v1_0_2_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionGeneralTypes_v1_0_0_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionHaulOffNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PlasticsRubberExtrusionMeltPump nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberGeneralTypes_v1_0_2_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionGeneralTypes_v1_0_0_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionMeltPumpNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PlasticsRubberExtrusionPelletizer nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberGeneralTypes_v1_0_2_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionGeneralTypes_v1_0_0_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionPelletizerNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PlasticsRubberExtrusionv2Calender nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadMachineryNodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberGeneralTypes_v1_0_3_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionv2GeneralTypesNodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionv2CalenderNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PlasticsRubberExtrusionv2Calibrator nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadMachineryNodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberGeneralTypes_v1_0_3_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionv2GeneralTypesNodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionv2CalibratorNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PlasticsRubberExtrusionv2Corrugator nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadMachineryNodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberGeneralTypes_v1_0_3_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionv2GeneralTypesNodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionv2HaulOffNodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionv2CorrugatorNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PlasticsRubberExtrusionv2Cutter nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadMachineryNodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberGeneralTypes_v1_0_3_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionv2GeneralTypesNodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionv2CutterNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PlasticsRubberExtrusionv2Die nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadMachineryNodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberGeneralTypes_v1_0_3_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionv2GeneralTypesNodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionv2DieNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PlasticsRubberExtrusionv2Extruder nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadMachineryNodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberGeneralTypes_v1_0_3_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionv2GeneralTypesNodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionv2ExtruderNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PlasticsRubberExtrusionv2ExtrusionLine nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadMachineryNodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberGeneralTypes_v1_0_3_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionv2GeneralTypesNodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionv2ExtrusionLineNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PlasticsRubberExtrusionv2Filter nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadMachineryNodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberGeneralTypes_v1_0_3_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionv2GeneralTypesNodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionv2FilterNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PlasticsRubberExtrusionv2GeneralTypes nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadMachineryNodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberGeneralTypes_v1_0_3_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionv2GeneralTypesNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PlasticsRubberExtrusionv2HaulOff nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadMachineryNodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberGeneralTypes_v1_0_3_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionv2GeneralTypesNodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionv2HaulOffNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PlasticsRubberExtrusionv2MeltPump nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadMachineryNodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberGeneralTypes_v1_0_3_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionv2GeneralTypesNodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionv2MeltPumpNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PlasticsRubberExtrusionv2Pelletizer nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadMachineryNodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberGeneralTypes_v1_0_3_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionv2GeneralTypesNodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberExtrusionv2PelletizerNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PlasticsRubberGeneralTypes v1.02 nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberGeneralTypes_v1_0_2_Nodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PlasticsRubberGeneralTypes v1.03 nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberGeneralTypes_v1_0_3_Nodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PlasticsRubberHotRunner nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberGeneralTypes_v1_0_3_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberHotRunnerNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PlasticsRubberIMM2MES nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberGeneralTypes_v1_0_2_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberIMM2MESNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PlasticsRubberLDS nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberGeneralTypes_v1_0_3_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberLDSNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PlasticsRubberTCD nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberGeneralTypes_v1_0_2_Nodeset);
        tcase_add_test(tc_server, Server_loadPlasticsRubberTCDNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PLCopen nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadPLCopenNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PNRIO nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadPnRioNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PNEM nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadPnEmNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load PROFINET nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadPROFINETNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load Robotics nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadRoboticsNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load Safety nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadSafetyNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load Sercos nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadSercosNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load Weihenstephan nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadMachineryNodeset);
        tcase_add_test(tc_server, Server_loadPackMLNodeset);
        tcase_add_test(tc_server, Server_loadWeihenstephanNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load WoodworkingEumabois nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadMachineryNodeset);
        tcase_add_test(tc_server, Server_loadWoodworkingNodeset);
        tcase_add_test(tc_server, Server_loadWoodworkingEumaboisNodeset);
        suite_add_tcase(s, tc_server);
    }
    {
        TCase *tc_server = tcase_create("Server load Woodworking nodeset");
        tcase_add_unchecked_fixture(tc_server, setup, teardown);
        tcase_add_test(tc_server, Server_loadDINodeset);
        tcase_add_test(tc_server, Server_loadMachineryNodeset);
        tcase_add_test(tc_server, Server_loadWoodworkingNodeset);
        suite_add_tcase(s, tc_server);
    }
    return s;
}

int main(void) {
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
