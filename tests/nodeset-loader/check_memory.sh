#!/usr/bin/env bash

# Test case for all standardized companion nodesets. You can
# find the specifications at https://github.com/OPCFoundation/UA-Nodeset.
#      - branch: latest
#      - commit: a2208e8
# 
# Currently this test case is missing the following UA-Nodesets:
#   * NodesetLoader related issues:
#      - ${OPEN62541_NODESET_DIR}AML/Opc.Ua.AMLLibraries.NodeSet2.xml
#      - ${OPEN62541_NODESET_DIR}ISA95-JOBCONTROL/opc.ua.isa95-jobcontrol.nodeset2.xml
#      - ${OPEN62541_NODESET_DIR}PNRIO/Opc.Ua.PnRio.Nodeset2.xml
#      - ${OPEN62541_NODESET_DIR}TMC/Opc.Ua.TMC.NodeSet2.xml
#
#   * Memory leak issues (libXML2 library related):
#      - ${OPEN62541_NODESET_DIR}CNC/Opc.Ua.CNC.NodeSet.xml
#      - ${OPEN62541_NODESET_DIR}IJT/Tightening/Opc.Ua.Ijt.Tightening.NodeSet2.xml
#      - $(OPEN62541_NODESET_DIR)MachineVision/Opc.Ua.MachineVision.NodeSet2.xml
#      - $(OPEN62541_NODESET_DIR)Mining/General/1.0.0/Opc.Ua.Mining.General.NodeSet2.xml
#          - Dependent Nodesets:
#             * $(OPEN62541_NODESET_DIR)Mining/DevelopmentSupport/Dozer/1.0.0/Opc.Ua.Mining.DevelopmentSupport.Dozer.NodeSet2.xml
#             * $(OPEN62541_NODESET_DIR)Mining/DevelopmentSupport/RoofSupportSystem/1.0.0/Opc.Ua.Mining.DevelopmentSupport.RoofSupportSystem.NodeSet2.xml
#             * $(OPEN62541_NODESET_DIR)Mining/Extraction/ShearerLoader/1.0.0/Opc.Ua.Mining.Extraction.ShearerLoader.NodeSet2.xml
#             * $(OPEN62541_NODESET_DIR)Mining/Loading/General/1.0.0/Opc.Ua.Mining.Loading.General.NodeSet2.xml
#             * $(OPEN62541_NODESET_DIR)Mining/Loading/HydraulicExcavator/1.0.0/Opc.Ua.Mining.Loading.HydraulicExcavator.NodeSet2.xml
#             * $(OPEN62541_NODESET_DIR)Mining/MineralProcessing/RockCrusher/1.0.0/Opc.Ua.Mining.MineralProcessing.RockCrusher.NodeSet2.xml
#             * $(OPEN62541_NODESET_DIR)Mining/PELOServices/FaceAlignmentSystem/1.0.0/Opc.Ua.Mining.PELOServices.FaceAlignmentSystem.NodeSet2.xml
#             * $(OPEN62541_NODESET_DIR)Mining/PELOServices/General/1.0.0/Opc.Ua.Mining.PELOServices.General.NodeSet2.xml
#             * $(OPEN62541_NODESET_DIR)Mining/TransportDumping/ArmouredFaceConveyor/1.0.0/Opc.Ua.Mining.TransportDumping.ArmouredFaceConveyor.NodeSet2.xml
#             * $(OPEN62541_NODESET_DIR)Mining/TransportDumping/General/1.0.0/Opc.Ua.Mining.TransportDumping.General.NodeSet2.xml
#             * $(OPEN62541_NODESET_DIR)Mining/TransportDumping/RearDumpTruck/1.0.0/Opc.Ua.Mining.TransportDumping.RearDumpTruck.NodeSet2.xml
#      - $(OPEN62541_NODESET_DIR)PADIM/Opc.Ua.IRDI.NodeSet2.xml
#          - Dependent Nodesets:
#             * $(OPEN62541_NODESET_DIR)PADIM/Opc.Ua.PADIM.NodeSet2.xml
#      - $(OPEN62541_NODESET_DIR)POWERLINK/Opc.Ua.POWERLINK.NodeSet2.xml
#      - $(OPEN62541_NODESET_DIR)Pumps/Opc.Ua.Pumps.NodeSet2.xml
#      - $(OPEN62541_NODESET_DIR)Scales/Opc.Ua.Scales.NodeSet2.xml
#

set -e
IFS=

function print_nodesets() {

    echo "Nodesets:"
    shift

    for i in $@
    do
        echo "      $(basename $i)"
    done
}

function total_memory_usage() {

    echo -ne "Total memory usage: "

    TEST_APP=$1
    shift

    touch tmp.log
    valgrind --trace-children=yes --tool=memcheck --log-file=tmp.log $TEST_APP $@ &> /dev/null
    grep -o -P '(?<=frees,\ ).*(?=\ bytes\ allocated)' tmp.log | echo "$(tail -n 1)" "B"
    rm tmp.log
}

function memory_usage() {
    echo -en "|\n-> "
    print_nodesets $@
    echo -en "|\n-> "
    total_memory_usage $@
    echo; echo
}

UA_NODESET_PATH=/usr/local/share/open62541/tools/ua-nodeset
NODESETLOADER_TESTS_PATH=./../../build/bin/tests
NODESETLOADER_TEST_APP=check_nodeset_loader_input

echo "... ADI ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/ADI/Opc.Ua.Adi.NodeSet2.xml

echo "... AMB ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/AMB/Opc.Ua.AMB.NodeSet2.xml

echo "... AMLBaseTypes ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/AML/Opc.Ua.AMLBaseTypes.NodeSet2.xml

echo "... AutoID ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/AutoID/Opc.Ua.AutoID.NodeSet2.xml

echo "... CAS ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/IA/Opc.Ua.IA.NodeSet2.xml \
             $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
             $UA_NODESET_PATH/CAS/Opc.Ua.CAS.NodeSet2.xml

echo "... CommercialKitchenEquipment ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/CommercialKitchenEquipment/Opc.Ua.CommercialKitchenEquipment.NodeSet2.xml

echo "... CSPPlusForMachine ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/CSPPlusForMachine/Opc.Ua.CSPPlusForMachine.NodeSet2.xml

echo "... DEXPI ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DEXPI/Opc.Ua.DEXPI.NodeSet2.xml

echo "... DI ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml

echo "... DotNet ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DotNet/Opc.Ua.NodeSet.xml

echo "... FDI5 ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/FDI/Opc.Ua.Fdi5.NodeSet2.xml

echo "... FDI7 ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/FDI/Opc.Ua.Fdi7.NodeSet2.xml

echo "... FDT ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/FDT/Opc.Ua.FDT.NodeSet.xml 

echo "... GDS ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/GDS/Opc.Ua.Gds.NodeSet2.xml

echo "... GDS.Part12 ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/GDS/Opc.Ua.Gds.NodeSet2.Part12.xml

echo "... Flat.Glass ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
             $UA_NODESET_PATH/Glass/Flat/Opc.Ua.Glass.NodeSet2.xml

echo "... I4AAS ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/I4AAS/Opc.Ua.I4AAS.NodeSet2.xml

echo "... IA ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/IA/Opc.Ua.IA.NodeSet2.xml

echo "... IA.examples ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/IA/Opc.Ua.IA.NodeSet2.xml \
             $UA_NODESET_PATH/IA/Opc.Ua.IA.NodeSet2.examples.xml

echo "... IOLinkIODD ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/IOLink/Opc.Ua.IOLinkIODD.NodeSet2.xml

echo "... IOLink ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/IOLink/Opc.Ua.IOLink.NodeSet2.xml

echo "... ISA95 ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/ISA-95/Opc.ISA95.NodeSet2.xml

echo "... Machinery ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml

echo "... Machinery.Examples ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
             $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.Examples.NodeSet2.xml 

echo "... MachineTool ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
             $UA_NODESET_PATH/IA/Opc.Ua.IA.NodeSet2.xml \
             $UA_NODESET_PATH/MachineTool/Opc.Ua.MachineTool.NodeSet2.xml

echo "... MDIS ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/MDIS/Opc.MDIS.NodeSet2.xml

echo "... Mining.DevelopmentSupport.General ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/Mining/DevelopmentSupport/General/1.0.0/Opc.Ua.Mining.DevelopmentSupport.General.NodeSet2.xml

echo "... Mining.Extraction.General ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/Mining/Extraction/General/1.0.0/Opc.Ua.Mining.Extraction.General.NodeSet2.xml

echo "... Mining.MineralProcessing.General ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/Mining/MineralProcessing/General/1.0.0/Opc.Ua.Mining.MineralProcessing.General.NodeSet2.xml

echo "... Mining.MonitoringSupervisionServices.General ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/Mining/MonitoringSupervisionServices/General/1.0.0/Opc.Ua.Mining.MonitoringSupervisionServices.General.NodeSet2.xml

echo "... MTConnect ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/MTConnect/Opc.Ua.MTConnect.NodeSet2.xml

echo "... OpenSCS ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/OpenSCS/Opc.Ua.OPENSCS.NodeSet2.xml

echo "... PackML ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/PackML/Opc.Ua.PackML.NodeSet2.xml

echo "... PlasticsRubber.Extrusion.Calender ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion/GeneralTypes/1.01/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion/Calender/1.00/Opc.Ua.PlasticsRubber.Extrusion.Calender.NodeSet2.xml

echo "... PlasticsRubber.Extrusion.Calibrator ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion/GeneralTypes/1.00/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion/Calibrator/1.00/Opc.Ua.PlasticsRubber.Extrusion.Calibrator.NodeSet2.xml

echo "... PlasticsRubber.Extrusion.Corrugator ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion/GeneralTypes/1.00/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion/HaulOff/1.00/Opc.Ua.PlasticsRubber.Extrusion.HaulOff.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion/Corrugator/1.00/Opc.Ua.PlasticsRubber.Extrusion.Corrugator.NodeSet2.xml

echo "... PlasticsRubber.Extrusion.Cutter ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion/GeneralTypes/1.00/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion/Cutter/1.00/Opc.Ua.PlasticsRubber.Extrusion.Cutter.NodeSet2.xml

echo "... PlasticsRubber.Extrusion.Die ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion/GeneralTypes/1.00/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion/Die/1.00/Opc.Ua.PlasticsRubber.Extrusion.Die.NodeSet2.xml

echo "... PlasticsRubber.Extrusion.Extruder ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion/GeneralTypes/1.00/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion/Extruder/1.00/Opc.Ua.PlasticsRubber.Extrusion.Extruder.NodeSet2.xml

echo "... PlasticsRubber.Extrusion.ExtrusionLine ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion/ExtrusionLine/1.00/Opc.Ua.PlasticsRubber.Extrusion.ExtrusionLine.NodeSet2.xml

echo "... PlasticsRubber.Extrusion.Filter ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion/GeneralTypes/1.00/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion/Filter/1.00/Opc.Ua.PlasticsRubber.Extrusion.Filter.NodeSet2.xml

echo "... PlasticsRubber.Extrusion.GeneralTypes ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion/GeneralTypes/1.00/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml

echo "1.01/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion/GeneralTypes/1.01/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml

echo "... PlasticsRubber.Extrusion.HaulOff ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion/GeneralTypes/1.00/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion/HaulOff/1.00/Opc.Ua.PlasticsRubber.Extrusion.HaulOff.NodeSet2.xml

echo "... PlasticsRubber.Extrusion.MeltPump ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion/GeneralTypes/1.00/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion/MeltPump/1.00/Opc.Ua.PlasticsRubber.Extrusion.MeltPump.NodeSet2.xml

echo "... PlasticsRubber.Extrusion.Pelletizer ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion/GeneralTypes/1.00/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion/Pelletizer/1.00/Opc.Ua.PlasticsRubber.Extrusion.Pelletizer.NodeSet2.xml

echo "... PlasticsRubber.Extrusion_v2.Calender ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/Calender/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Calender.NodeSet2.xml

echo "... PlasticsRubber.Extrusion_v2.Calibrator ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/Calibrator/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Calibrator.NodeSet2.xml

echo "... PlasticsRubber.Extrusion_v2.Corrugator ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/HaulOff/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.HaulOff.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/Corrugator/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Corrugator.NodeSet2.xml

echo "... PlasticsRubber.Extrusion_v2.Cutter ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/Cutter/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Cutter.NodeSet2.xml

echo "... PlasticsRubber.Extrusion_v2.Die ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/Die/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Die.NodeSet2.xml

echo "... PlasticsRubber.Extrusion_v2.Extruder ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/Extruder/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Extruder.NodeSet2.xml

echo "... PlasticsRubber.Extrusion_v2.ExtrusionLine ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/ExtrusionLine/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.ExtrusionLine.NodeSet2.xml

echo "... PlasticsRubber.Extrusion_v2.Filter ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/Filter/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Filter.NodeSet2.xml

echo "... PlasticsRubber.Extrusion_v2.GeneralTypes ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml

echo "... PlasticsRubber.Extrusion_v2.HaulOff ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/HaulOff/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.HaulOff.NodeSet2.xml

echo "... PlasticsRubber.Extrusion_v2.MeltPump ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/MeltPump/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.MeltPump.NodeSet2.xml

echo "... PlasticsRubber.Extrusion_v2.Pelletizer ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/Pelletizer/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Pelletizer.NodeSet2.xml

echo "... PlasticsRubber.GeneralTypes.1.02 ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml

echo "... PlasticsRubber.GeneralTypes.1.03 ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml

echo "... PlasticsRubber.HotRunner ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/HotRunner/1.00/Opc.Ua.PlasticsRubber.HotRunner.NodeSet2.xml

echo "... PlasticsRubber.IMM2MES ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/IMM2MES/1.01/Opc.Ua.PlasticsRubber.IMM2MES.NodeSet2.xml

echo "... PlasticsRubber.LDS ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/LDS/1.00/Opc.Ua.PlasticsRubber.LDS.NodeSet2.xml

echo "... PlasticsRubber.TCD ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
             $UA_NODESET_PATH/PlasticsRubber/TCD/1.01/Opc.Ua.PlasticsRubber.TCD.NodeSet2.xml

echo "... PLCopen.V1.02 ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/PLCopen/Opc.Ua.PLCopen.NodeSet2_V1.02.xml

echo "... PNEM ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/PNEM/Opc.Ua.PnEm.NodeSet2.xml

echo "... PROFINET ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/PROFINET/Opc.Ua.Pn.NodeSet2.xml

echo "... Robotics ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/Robotics/Opc.Ua.Robotics.NodeSet2.xml

echo "... Safety ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/Safety/Opc.Ua.Safety.NodeSet2.xml

echo "... Sercos ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/Sercos/Sercos.NodeSet2.xml

echo "... Weihenstephan ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
             $UA_NODESET_PATH/PackML/Opc.Ua.PackML.NodeSet2.xml \
             $UA_NODESET_PATH/Weihenstephan/Opc.Ua.Weihenstephan.NodeSet2.xml

echo "... Woodworking.Eumabois ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
             $UA_NODESET_PATH/Woodworking/Opc.Ua.Woodworking.NodeSet2.xml \
             $UA_NODESET_PATH/Woodworking/Opc.Ua.Eumabois.Nodeset2.xml

echo "... Woodworking ..."
memory_usage $NODESETLOADER_TESTS_PATH/$NODESETLOADER_TEST_APP \
             $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
             $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
             $UA_NODESET_PATH/Woodworking/Opc.Ua.Woodworking.NodeSet2.xml