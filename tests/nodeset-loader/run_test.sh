#!/usr/bin/env bash

# Test case for all standardized companion nodesets. You can
# find the specifications at https://github.com/OPCFoundation/UA-Nodeset.
#      - branch: latest
#      - commit: 54e3513
# 
# Currently this test case is missing the following UA-Nodesets:
#   * NodesetLoader related issues:
#      - ${OPEN62541_NODESET_DIR}AML/Opc.Ua.AMLLibraries.NodeSet2.xml
#      - ${OPEN62541_NODESET_DIR}CNC/Opc.Ua.CNC.NodeSet.xml
#      - ${OPEN62541_NODESET_DIR}ISA95-JOBCONTROL/opc.ua.isa95-jobcontrol.nodeset2.xml
#      - ${OPEN62541_NODESET_DIR}TMC/Opc.Ua.TMC.NodeSet2.xml
#
#   * NodesetLoader XML decoding not supported for certain types issues:
#      - ${OPEN62541_NODESET_DIR}IJT/Tightening/Opc.Ua.Ijt.Tightening.NodeSet2.xml
#      - ${OPEN62541_NODESET_DIR}MachineVision/Opc.Ua.MachineVision.NodeSet2.xml
#      - ${OPEN62541_NODESET_DIR}Pumps/Opc.Ua.Pumps.NodeSet2.xml
#
#   * Nodesets with known 'AddNode()' issues:
#      - ${OPEN62541_NODESET_DIR}AML/Opc.Ua.AMLBaseTypes.NodeSet2.xml
#      - ${OPEN62541_NODESET_DIR}CSPPlusForMachine/Opc.Ua.CSPPlusForMachine.NodeSet2.xml
#      - ${OPEN62541_NODESET_DIR}FDI/Opc.Ua.Fdi5.NodeSet2.xml
#      - ${OPEN62541_NODESET_DIR}FDI/Opc.Ua.Fdi7.NodeSet2.xml
#      - ${OPEN62541_NODESET_DIR}ISA-95/Opc.ISA95.NodeSet2.xml
#      - ${OPEN62541_NODESET_DIR}POWERLINK/Opc.Ua.POWERLINK.NodeSet2.xml
#
#
# Adding new entry within the integration test:
#
#       add_node_integration_test [arbitrarily_defined_nodeset_name] \
#           [nodeset_xml_path1] \
#           [nodeset_xml_path2 ...]
#

IFS=

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" && pwd )

argv=("$@")
argc=$#
CLIENT_BINARY_PATH="${argv[0]}"
SERVER_BINARY_PATH="${argv[1]}"
SERVER_PID=
CustomNsStartNum=
declare -a NodeIdFiles=()

function start_server() {
    echo "Start server: "$@
    $SERVER_BINARY_PATH $@ & disown
    SERVER_PID=$!
}

function prepare_nodeids() {
    CustomNsStartNum=2
    for nodeset in "$@"
    do
        nodesetIdPath=$(basename $nodeset)
        nodesetIdPath=${nodesetIdPath%.*}
        if [[ $nodesetIdPath == *"MTConnect"* ]]; then
            grep --line-buffered -R '<UAObjectType\|<UAObject\|<UAVariable\|<UAVariableType\|<UAMethod\|<UADataType\|<UAReferenceType\|<UAView' \
                                    $nodeset | awk '{ sub(/.*\ NodeId=\x27/, ""); sub(/\x27.*/, ""); print $1}' >> $nodesetIdPath
        else
            grep --line-buffered -R '<UAObjectType\|<UAObject\|<UAVariable\|<UAVariableType\|<UAMethod\|<UADataType\|<UAReferenceType\|<UAView' \
                                    $nodeset | awk '{ sub(/.*\ NodeId="/, ""); sub(/".*/, ""); print $1}' >> $nodesetIdPath
        fi
        
        if [[ $nodesetIdPath == *"PLCopen"* ]]; then
            sed -i "s/ns=2;/ns=$CustomNsStartNum;/gI" $nodesetIdPath
            sed -i "s/ns=1;/ns=2;/gI" $nodesetIdPath
        elif [[ $nodesetIdPath == *"PADIM"* ]]; then
            sed -i "s/ns=3;/ns=$CustomNsStartNum;/gI" $nodesetIdPath
        elif [[ $nodesetIdPath == *"Weihenstephan"* ]]; then
            sed -i "s/ns=4;/ns=$CustomNsStartNum;/gI" $nodesetIdPath
        else
            sed -i "s/ns=1;/ns=$CustomNsStartNum;/gI" $nodesetIdPath
        fi

        NodeIdFiles+=("$nodesetIdPath")
        CustomNsStartNum=$((CustomNsStartNum+1))
    done
}

function run_client() {
    echo "Start client: "$@
    $CLIENT_BINARY_PATH $@
    clientResult=$?
    if [ $clientResult -ne 0 ] ; then
        stop_server
        rm ${NodeIdFiles[@]}
        exit 1
    fi
}

function stop_server() {
    echo "Stop server:"
    kill -9 $SERVER_PID
    if [ $? -ne 0 ] ; then
        echo "Error: stopping the server failed"
        echo "PID = " $SERVER_PID
        rm ${NodeIdFiles[@]}
        exit 1
    fi
}

function add_node_integration_test() {
    nodesetName=$1
    shift 1
    prepare_nodeids $@
    start_server $@
    sleep 3
    run_client $nodesetName ${NodeIdFiles[@]}
    stop_server
    rm ${NodeIdFiles[@]}
    NodeIdFiles=()
}

UA_NODESET_PATH=$SCRIPT_DIR/../../deps/ua-nodeset

add_node_integration_test "ADI" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/ADI/Opc.Ua.Adi.NodeSet2.xml

add_node_integration_test "AMB" \
    $UA_NODESET_PATH/AMB/Opc.Ua.AMB.NodeSet2.xml

add_node_integration_test "AutoID" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/AutoID/Opc.Ua.AutoID.NodeSet2.xml

add_node_integration_test "BACnet" \
    $UA_NODESET_PATH/BACnet/Opc.Ua.BACnet.NodeSet2.xml

add_node_integration_test "CAS" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/IA/Opc.Ua.IA.NodeSet2.xml \
    $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
    $UA_NODESET_PATH/CAS/Opc.Ua.CAS.NodeSet2.xml

add_node_integration_test "CommercialKitchenEquipment" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/CommercialKitchenEquipment/Opc.Ua.CommercialKitchenEquipment.NodeSet2.xml

add_node_integration_test "DEXPI" \
    $UA_NODESET_PATH/DEXPI/Opc.Ua.DEXPI.NodeSet2.xml

add_node_integration_test "DI" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml

add_node_integration_test "DotNet" \
    $UA_NODESET_PATH/DotNet/Opc.Ua.NodeSet.xml

add_node_integration_test "FDT" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/FDT/Opc.Ua.FDT.NodeSet.xml

add_node_integration_test "GDS" \
    $UA_NODESET_PATH/GDS/Opc.Ua.Gds.NodeSet2.xml

add_node_integration_test "Flat.Glass" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
    $UA_NODESET_PATH/Glass/Flat/Opc.Ua.Glass.NodeSet2.xml

add_node_integration_test "I4AAS" \
    $UA_NODESET_PATH/I4AAS/Opc.Ua.I4AAS.NodeSet2.xml

add_node_integration_test "IA" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/IA/Opc.Ua.IA.NodeSet2.xml

add_node_integration_test "IA.examples" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/IA/Opc.Ua.IA.NodeSet2.xml \
    $UA_NODESET_PATH/IA/Opc.Ua.IA.NodeSet2.examples.xml

add_node_integration_test "IOLinkIODD" \
    $UA_NODESET_PATH/IOLink/Opc.Ua.IOLinkIODD.NodeSet2.xml

add_node_integration_test "IOLink" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/IOLink/Opc.Ua.IOLink.NodeSet2.xml

add_node_integration_test "Machinery" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml

add_node_integration_test "Machinery.Examples" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
    $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.Examples.NodeSet2.xml

add_node_integration_test "MachineTool" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
    $UA_NODESET_PATH/IA/Opc.Ua.IA.NodeSet2.xml \
    $UA_NODESET_PATH/MachineTool/Opc.Ua.MachineTool.NodeSet2.xml

# add_node_integration_test "MDIS" \
#     $UA_NODESET_PATH/MDIS/Opc.MDIS.NodeSet2.xml

add_node_integration_test "Mining.DevelopmentSupport.Dozer" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
    $UA_NODESET_PATH/Mining/General/1.0.0/Opc.Ua.Mining.General.NodeSet2.xml \
    $UA_NODESET_PATH/Mining/DevelopmentSupport/Dozer/1.0.0/Opc.Ua.Mining.DevelopmentSupport.Dozer.NodeSet2.xml

add_node_integration_test "Mining.DevelopmentSupport.General" \
    $UA_NODESET_PATH/Mining/DevelopmentSupport/General/1.0.0/Opc.Ua.Mining.DevelopmentSupport.General.NodeSet2.xml

add_node_integration_test "Mining.DevelopmentSupport.RoofSupportSystem" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
    $UA_NODESET_PATH/Mining/General/1.0.0/Opc.Ua.Mining.General.NodeSet2.xml \
    $UA_NODESET_PATH/Mining/DevelopmentSupport/RoofSupportSystem/1.0.0/Opc.Ua.Mining.DevelopmentSupport.RoofSupportSystem.NodeSet2.xml

add_node_integration_test "Mining.Extraction.General" \
    $UA_NODESET_PATH/Mining/Extraction/General/1.0.0/Opc.Ua.Mining.Extraction.General.NodeSet2.xml

add_node_integration_test "Mining.Extraction.ShearerLoader" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
    $UA_NODESET_PATH/Mining/General/1.0.0/Opc.Ua.Mining.General.NodeSet2.xml \
    $UA_NODESET_PATH/Mining/Extraction/ShearerLoader/1.0.0/Opc.Ua.Mining.Extraction.ShearerLoader.NodeSet2.xml

add_node_integration_test "Mining.General" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
    $UA_NODESET_PATH/Mining/General/1.0.0/Opc.Ua.Mining.General.NodeSet2.xml

add_node_integration_test "Mining.Loading.General" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
    $UA_NODESET_PATH/Mining/General/1.0.0/Opc.Ua.Mining.General.NodeSet2.xml \
    $UA_NODESET_PATH/Mining/Loading/General/1.0.0/Opc.Ua.Mining.Loading.General.NodeSet2.xml

add_node_integration_test "Mining.Loading.HydraulicExcavator" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
    $UA_NODESET_PATH/Mining/General/1.0.0/Opc.Ua.Mining.General.NodeSet2.xml \
    $UA_NODESET_PATH/Mining/Loading/General/1.0.0/Opc.Ua.Mining.Loading.General.NodeSet2.xml \
    $UA_NODESET_PATH/Mining/Loading/HydraulicExcavator/1.0.0/Opc.Ua.Mining.Loading.HydraulicExcavator.NodeSet2.xml

add_node_integration_test "Mining.MineralProcessing.General" \
    $UA_NODESET_PATH/Mining/MineralProcessing/General/1.0.0/Opc.Ua.Mining.MineralProcessing.General.NodeSet2.xml

add_node_integration_test "Mining.MineralProcessing.RockCrusher" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
    $UA_NODESET_PATH/Mining/General/1.0.0/Opc.Ua.Mining.General.NodeSet2.xml \
    $UA_NODESET_PATH/Mining/MineralProcessing/RockCrusher/1.0.0/Opc.Ua.Mining.MineralProcessing.RockCrusher.NodeSet2.xml

add_node_integration_test "Mining.MonitoringSupervisionServices.General" \
    $UA_NODESET_PATH/Mining/MonitoringSupervisionServices/General/1.0.0/Opc.Ua.Mining.MonitoringSupervisionServices.General.NodeSet2.xml

add_node_integration_test "Mining.PELOServices.FaceAlignmentSystem" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
    $UA_NODESET_PATH/Mining/General/1.0.0/Opc.Ua.Mining.General.NodeSet2.xml \
    $UA_NODESET_PATH/Mining/PELOServices/FaceAlignmentSystem/1.0.0/Opc.Ua.Mining.PELOServices.FaceAlignmentSystem.NodeSet2.xml

add_node_integration_test "Mining.PELOServices.General" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
    $UA_NODESET_PATH/Mining/General/1.0.0/Opc.Ua.Mining.General.NodeSet2.xml \
    $UA_NODESET_PATH/Mining/PELOServices/General/1.0.0/Opc.Ua.Mining.PELOServices.General.NodeSet2.xml

add_node_integration_test "Mining.TransportDumping.ArmouredFaceConveyor" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
    $UA_NODESET_PATH/Mining/General/1.0.0/Opc.Ua.Mining.General.NodeSet2.xml \
    $UA_NODESET_PATH/Mining/TransportDumping/ArmouredFaceConveyor/1.0.0/Opc.Ua.Mining.TransportDumping.ArmouredFaceConveyor.NodeSet2.xml

add_node_integration_test "Mining.TransportDumping.General" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
    $UA_NODESET_PATH/Mining/General/1.0.0/Opc.Ua.Mining.General.NodeSet2.xml \
    $UA_NODESET_PATH/Mining/TransportDumping/General/1.0.0/Opc.Ua.Mining.TransportDumping.General.NodeSet2.xml

add_node_integration_test "Mining.TransportDumping.RearDumpTruck" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
    $UA_NODESET_PATH/Mining/General/1.0.0/Opc.Ua.Mining.General.NodeSet2.xml \
    $UA_NODESET_PATH/Mining/TransportDumping/General/1.0.0/Opc.Ua.Mining.TransportDumping.General.NodeSet2.xml \
    $UA_NODESET_PATH/Mining/TransportDumping/RearDumpTruck/1.0.0/Opc.Ua.Mining.TransportDumping.RearDumpTruck.NodeSet2.xml

add_node_integration_test "MTConnect" \
    $UA_NODESET_PATH/MTConnect/Opc.Ua.MTConnect.NodeSet2.xml

add_node_integration_test "OpenSCS" \
    $UA_NODESET_PATH/OpenSCS/Opc.Ua.OPENSCS.NodeSet2.xml

add_node_integration_test "PackML" \
    $UA_NODESET_PATH/PackML/Opc.Ua.PackML.NodeSet2.xml

add_node_integration_test "PADIM.IRDI" \
    $UA_NODESET_PATH/PADIM/Opc.Ua.IRDI.NodeSet2.xml

add_node_integration_test "PADIM" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/PADIM/Opc.Ua.IRDI.NodeSet2.xml \
    $UA_NODESET_PATH/PADIM/Opc.Ua.PADIM.NodeSet2.xml

add_node_integration_test "PlasticsRubber.Extrusion.Calender" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion/GeneralTypes/1.01/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion/Calender/1.00/Opc.Ua.PlasticsRubber.Extrusion.Calender.NodeSet2.xml

add_node_integration_test "PlasticsRubber.Extrusion.Calibrator" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion/GeneralTypes/1.00/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion/Calibrator/1.00/Opc.Ua.PlasticsRubber.Extrusion.Calibrator.NodeSet2.xml

add_node_integration_test "PlasticsRubber.Extrusion.Corrugator" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion/GeneralTypes/1.00/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion/HaulOff/1.00/Opc.Ua.PlasticsRubber.Extrusion.HaulOff.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion/Corrugator/1.00/Opc.Ua.PlasticsRubber.Extrusion.Corrugator.NodeSet2.xml

add_node_integration_test "PlasticsRubber.Extrusion.Cutter" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion/GeneralTypes/1.00/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion/Cutter/1.00/Opc.Ua.PlasticsRubber.Extrusion.Cutter.NodeSet2.xml

add_node_integration_test "PlasticsRubber.Extrusion.Die" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion/GeneralTypes/1.00/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion/Die/1.00/Opc.Ua.PlasticsRubber.Extrusion.Die.NodeSet2.xml

add_node_integration_test "PlasticsRubber.Extrusion.Extruder" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion/GeneralTypes/1.00/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion/Extruder/1.00/Opc.Ua.PlasticsRubber.Extrusion.Extruder.NodeSet2.xml

add_node_integration_test "PlasticsRubber.Extrusion.ExtrusionLine" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion/ExtrusionLine/1.00/Opc.Ua.PlasticsRubber.Extrusion.ExtrusionLine.NodeSet2.xml

add_node_integration_test "PlasticsRubber.Extrusion.Filter" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion/GeneralTypes/1.00/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion/Filter/1.00/Opc.Ua.PlasticsRubber.Extrusion.Filter.NodeSet2.xml

add_node_integration_test "PlasticsRubber.Extrusion.GeneralTypes" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion/GeneralTypes/1.00/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml

add_node_integration_test "1.01/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion/GeneralTypes/1.01/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml

add_node_integration_test "PlasticsRubber.Extrusion.HaulOff" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion/GeneralTypes/1.00/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion/HaulOff/1.00/Opc.Ua.PlasticsRubber.Extrusion.HaulOff.NodeSet2.xml

add_node_integration_test "PlasticsRubber.Extrusion.MeltPump" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion/GeneralTypes/1.00/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion/MeltPump/1.00/Opc.Ua.PlasticsRubber.Extrusion.MeltPump.NodeSet2.xml

add_node_integration_test "PlasticsRubber.Extrusion.Pelletizer" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion/GeneralTypes/1.00/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion/Pelletizer/1.00/Opc.Ua.PlasticsRubber.Extrusion.Pelletizer.NodeSet2.xml

add_node_integration_test "PlasticsRubber.Extrusion_v2.Calender" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/Calender/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Calender.NodeSet2.xml

add_node_integration_test "PlasticsRubber.Extrusion_v2.Calibrator" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/Calibrator/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Calibrator.NodeSet2.xml

add_node_integration_test "PlasticsRubber.Extrusion_v2.Corrugator" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/HaulOff/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.HaulOff.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/Corrugator/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Corrugator.NodeSet2.xml

add_node_integration_test "PlasticsRubber.Extrusion_v2.Cutter" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/Cutter/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Cutter.NodeSet2.xml

add_node_integration_test "PlasticsRubber.Extrusion_v2.Die" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/Die/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Die.NodeSet2.xml

add_node_integration_test "PlasticsRubber.Extrusion_v2.Extruder" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/Extruder/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Extruder.NodeSet2.xml

add_node_integration_test "PlasticsRubber.Extrusion_v2.ExtrusionLine" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/ExtrusionLine/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.ExtrusionLine.NodeSet2.xml

add_node_integration_test "PlasticsRubber.Extrusion_v2.Filter" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/Filter/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Filter.NodeSet2.xml

add_node_integration_test "PlasticsRubber.Extrusion_v2.GeneralTypes" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml

add_node_integration_test "PlasticsRubber.Extrusion_v2.HaulOff" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/HaulOff/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.HaulOff.NodeSet2.xml

add_node_integration_test "PlasticsRubber.Extrusion_v2.MeltPump" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/MeltPump/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.MeltPump.NodeSet2.xml

add_node_integration_test "PlasticsRubber.Extrusion_v2.Pelletizer" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/Extrusion_v2/Pelletizer/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Pelletizer.NodeSet2.xml

add_node_integration_test "PlasticsRubber.GeneralTypes.1.02" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml

add_node_integration_test "PlasticsRubber.GeneralTypes.1.03" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml

add_node_integration_test "PlasticsRubber.HotRunner" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/HotRunner/1.00/Opc.Ua.PlasticsRubber.HotRunner.NodeSet2.xml

add_node_integration_test "PlasticsRubber.IMM2MES" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/IMM2MES/1.01/Opc.Ua.PlasticsRubber.IMM2MES.NodeSet2.xml

add_node_integration_test "PlasticsRubber.LDS" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/LDS/1.00/Opc.Ua.PlasticsRubber.LDS.NodeSet2.xml

add_node_integration_test "PlasticsRubber.TCD" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
    $UA_NODESET_PATH/PlasticsRubber/TCD/1.01/Opc.Ua.PlasticsRubber.TCD.NodeSet2.xml

add_node_integration_test "PLCopen" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/PLCopen/Opc.Ua.PLCopen.NodeSet2_V1.02.xml

add_node_integration_test "PNEM" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/PNEM/Opc.Ua.PnEm.NodeSet2.xml

add_node_integration_test "PNRIO" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/PNRIO/Opc.Ua.PnRio.Nodeset2.xml

add_node_integration_test "PROFINET" \
    $UA_NODESET_PATH/PROFINET/Opc.Ua.Pn.NodeSet2.xml

add_node_integration_test "Robotics" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/Robotics/Opc.Ua.Robotics.NodeSet2.xml

add_node_integration_test "Safety" \
    $UA_NODESET_PATH/Safety/Opc.Ua.Safety.NodeSet2.xml

add_node_integration_test "Scales" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
    $UA_NODESET_PATH/PackML/Opc.Ua.PackML.NodeSet2.xml \
    $UA_NODESET_PATH/Scales/Opc.Ua.Scales.NodeSet2.xml

add_node_integration_test "Sercos" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/Sercos/Sercos.NodeSet2.xml

add_node_integration_test "Weihenstephan" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
    $UA_NODESET_PATH/PackML/Opc.Ua.PackML.NodeSet2.xml \
    $UA_NODESET_PATH/Weihenstephan/Opc.Ua.Weihenstephan.NodeSet2.xml

add_node_integration_test "Woodworking.Eumabois" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
    $UA_NODESET_PATH/Woodworking/Opc.Ua.Woodworking.NodeSet2.xml \
    $UA_NODESET_PATH/Woodworking/Opc.Ua.Eumabois.Nodeset2.xml

add_node_integration_test "Woodworking" \
    $UA_NODESET_PATH/DI/Opc.Ua.Di.NodeSet2.xml \
    $UA_NODESET_PATH/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
    $UA_NODESET_PATH/Woodworking/Opc.Ua.Woodworking.NodeSet2.xml
