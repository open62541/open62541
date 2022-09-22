#!/bin/bash

IFS=

output=
if [ "$1" = "0" ]; then
	output=/dev/stdout
elif [ "$1" = "1" ]; then
	output=
else
	if [ "$1" = "" ]; then
		echo -e "\nEmpty first argument\n"
	else
		echo -e "\nUnsupported first argument: "$1"\n"
	fi

	echo -e "USAGE:\n"
	echo -e "\t[0] - log parser output to 'standard output'"
	echo -e "\t[1] - log parser output to folders with a log file for each companion nodeset\n"
	exit
fi

NodesetsBasePath=/usr/local/share/open62541/tools/ua-nodeset
ExamplesBinPath=./../../build/bin/tests
TestAppName=check_nodeset_loader_input
LogsPath=$ExamplesBinPath/Logs

rm -rf $LogsPath
mkdir $LogsPath

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/ADI
	touch $LogsPath/ADI/ServerImportNodeSet2XmlTest_ADI_a2208e8.log
	output=$LogsPath/ADI/ServerImportNodeSet2XmlTest_ADI_a2208e8.log
fi
echo "... ADI ..." > $output
echo "Opc.Ua.Adi.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/ADI/Opc.Ua.Adi.NodeSet2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/AMB
	touch $LogsPath/AMB/ServerImportNodeSet2XmlTest_AMB_a2208e8.log
	output=$LogsPath/AMB/ServerImportNodeSet2XmlTest_AMB_a2208e8.log
fi
echo "... AMB ..." >> $output
echo "Opc.Ua.AMB.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/AMB/Opc.Ua.AMB.NodeSet2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/AML
	touch $LogsPath/AML/ServerImportNodeSet2XmlTest_AML_a2208e8.log
	output=$LogsPath/AML/ServerImportNodeSet2XmlTest_AML_a2208e8.log
fi
echo "... AML ..." >> $output
echo "Opc.Ua.AMLBaseTypes.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/AML/Opc.Ua.AMLBaseTypes.NodeSet2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/AutoID
	touch $LogsPath/AutoID/ServerImportNodeSet2XmlTest_AutoID_a2208e8.log
	output=$LogsPath/AutoID/ServerImportNodeSet2XmlTest_AutoID_a2208e8.log
fi
echo "... AutoID ..." >> $output
echo "Opc.Ua.AutoID.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/AutoID/Opc.Ua.AutoID.NodeSet2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/CAS
	touch $LogsPath/CAS/ServerImportNodeSet2XmlTest_CAS_a2208e8.log
	output=$LogsPath/CAS/ServerImportNodeSet2XmlTest_CAS_a2208e8.log
fi
echo "... CAS ..." >> $output
echo "Opc.Ua.CAS.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/IA/Opc.Ua.IA.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
						   $NodesetsBasePath/CAS/Opc.Ua.CAS.NodeSet2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/CNC
	touch $LogsPath/CNC/ServerImportNodeSet2XmlTest_CNC_a2208e8.log
	output=$LogsPath/CNC/ServerImportNodeSet2XmlTest_CNC_a2208e8.log
fi
echo "... CNC ..." >> $output
echo "Opc.Ua.CNC.NodeSet.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/CNC/Opc.Ua.CNC.NodeSet.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/CommercialKitchenEquipment
	touch $LogsPath/CommercialKitchenEquipment/ServerImportNodeSet2XmlTest_CommercialKitchenEquipment_a2208e8.log
	output=$LogsPath/CommercialKitchenEquipment/ServerImportNodeSet2XmlTest_CommercialKitchenEquipment_a2208e8.log
fi
echo "... CommercialKitchenEquipment ..." >> $output
echo "Opc.Ua.CommercialKitchenEquipment.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/CommercialKitchenEquipment/Opc.Ua.CommercialKitchenEquipment.NodeSet2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/CSPPlusForMachine
	touch $LogsPath/CSPPlusForMachine/ServerImportNodeSet2XmlTest_CSPPlusForMachine_a2208e8.log
	output=$LogsPath/CSPPlusForMachine/ServerImportNodeSet2XmlTest_CSPPlusForMachine_a2208e8.log
fi
echo "... CSPPlusForMachine ..." >> $output
echo "Opc.Ua.CSPPlusForMachine.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/CSPPlusForMachine/Opc.Ua.CSPPlusForMachine.NodeSet2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/DEXPI
	touch $LogsPath/DEXPI/ServerImportNodeSet2XmlTest_DEXPI_a2208e8.log
	output=$LogsPath/DEXPI/ServerImportNodeSet2XmlTest_DEXPI_a2208e8.log
fi
echo "... DEXPI ..." >> $output
echo "Opc.Ua.DEXPI.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DEXPI/Opc.Ua.DEXPI.NodeSet2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/DI
	touch $LogsPath/DI/ServerImportNodeSet2XmlTest_DI_a2208e8.log
	output=$LogsPath/DI/ServerImportNodeSet2XmlTest_DI_a2208e8.log
fi
echo "... DI ..." >> $output
echo "Opc.Ua.Di.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/DotNet
	touch $LogsPath/DotNet/ServerImportNodeSet2XmlTest_DotNet_a2208e8.log
	output=$LogsPath/DotNet/ServerImportNodeSet2XmlTest_DotNet_a2208e8.log
fi
echo "... DotNet ..." >> $output
echo "Opc.Ua.NodeSet.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DotNet/Opc.Ua.NodeSet.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/FDI
	touch $LogsPath/FDI/ServerImportNodeSet2XmlTest_FDI_a2208e8.log
	output=$LogsPath/FDI/ServerImportNodeSet2XmlTest_FDI_a2208e8.log
fi
echo "... FDI ..." >> $output
echo "Opc.Ua.Fdi5.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/FDI/Opc.Ua.Fdi5.NodeSet2.xml >> $output
echo >> $output
echo "Opc.Ua.Fdi7.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/FDI/Opc.Ua.Fdi7.NodeSet2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/FDT
	touch $LogsPath/FDT/ServerImportNodeSet2XmlTest_FDT_a2208e8.log
	output=$LogsPath/FDT/ServerImportNodeSet2XmlTest_FDT_a2208e8.log
fi
echo "... FDT ..." >> $output
echo "Opc.Ua.FDT.NodeSet.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/FDT/Opc.Ua.FDT.NodeSet.xml >> $output
echo >> $output; echo >> $output 

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/GDS
	touch $LogsPath/GDS/ServerImportNodeSet2XmlTest_GDS_a2208e8.log
	output=$LogsPath/GDS/ServerImportNodeSet2XmlTest_GDS_a2208e8.log
fi
echo "... GDS ..." >> $output
echo "Opc.Ua.GDS.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/GDS/Opc.Ua.Gds.NodeSet2.xml >> $output
echo >> $output
echo "Opc.Ua.GDS.NodeSet2.Part12.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/GDS/Opc.Ua.Gds.NodeSet2.Part12.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/Glass
	touch $LogsPath/Glass/ServerImportNodeSet2XmlTest_Glass_a2208e8.log
	output=$LogsPath/Glass/ServerImportNodeSet2XmlTest_Glass_a2208e8.log
fi
echo "... Glass ..." >> $output
echo "Flat/Opc.Ua.Glass.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
						   $NodesetsBasePath/Glass/Flat/Opc.Ua.Glass.NodeSet2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/I4AAS
	touch $LogsPath/I4AAS/ServerImportNodeSet2XmlTest_I4AAS_a2208e8.log
	output=$LogsPath/I4AAS/ServerImportNodeSet2XmlTest_I4AAS_a2208e8.log
fi
echo "... I4AAS ..." >> $output
echo "Opc.Ua.I4AAS.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/I4AAS/Opc.Ua.I4AAS.NodeSet2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/IA
	touch $LogsPath/IA/ServerImportNodeSet2XmlTest_IA_a2208e8.log
	output=$LogsPath/IA/ServerImportNodeSet2XmlTest_IA_a2208e8.log
fi
echo "... IA ..." >> $output
echo "Opc.Ua.IA.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/IA/Opc.Ua.IA.NodeSet2.xml >> $output
echo >> $output
echo "Opc.Ua.IA.NodeSet2.examples.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/IA/Opc.Ua.IA.NodeSet2.xml \
						   $NodesetsBasePath/IA/Opc.Ua.IA.NodeSet2.examples.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/IEC61850
	touch $LogsPath/IEC61850/ServerImportNodeSet2XmlTest_IEC61850_a2208e8.log
	output=$LogsPath/IEC61850/ServerImportNodeSet2XmlTest_IEC61850_a2208e8.log
fi
echo "... IEC61850 ..." >> $output
echo "Opc.Ua.IEC61850-6.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/IEC61850/Opc.Ua.IEC61850-7-3.NodeSet2.xml \
						   $NodesetsBasePath/IEC61850/Opc.Ua.IEC61850-6.NodeSet2.xml >> $output
echo >> $output
echo "Opc.Ua.IEC61850-7-3.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/IEC61850/Opc.Ua.IEC61850-7-3.NodeSet2.xml >> $output
echo >> $output
echo "Opc.Ua.IEC61850-7-4.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/IEC61850/Opc.Ua.IEC61850-7-3.NodeSet2.xml \
						   $NodesetsBasePath/IEC61850/Opc.Ua.IEC61850-7-4.NodeSet2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/IJT
	touch $LogsPath/IJT/ServerImportNodeSet2XmlTest_IJT_a2208e8.log
	output=$LogsPath/IJT/ServerImportNodeSet2XmlTest_IJT_a2208e8.log
fi
echo "... IJT ..." >> $output
echo "Tightening/Opc.Ua.Ijt.Tightening.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
						   $NodesetsBasePath/IJT/Tightening/Opc.Ua.Ijt.Tightening.NodeSet2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/IOLink
	touch $LogsPath/IOLink/ServerImportNodeSet2XmlTest_IOLink_a2208e8.log
	output=$LogsPath/IOLink/ServerImportNodeSet2XmlTest_IOLink_a2208e8.log
fi
echo "... IOLink ..." >> $output
echo "Opc.Ua.IOLinkIODD.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/IOLink/Opc.Ua.IOLinkIODD.NodeSet2.xml >> $output
echo >> $output
echo "Opc.Ua.IOLink.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/IOLink/Opc.Ua.IOLink.NodeSet2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/ISA-95
	touch $LogsPath/ISA-95/ServerImportNodeSet2XmlTest_ISA-95_a2208e8.log
	output=$LogsPath/ISA-95/ServerImportNodeSet2XmlTest_ISA-95_a2208e8.log
fi
echo "... ISA-95 ..." >> $output
echo "Opc.Ua.ISA95.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/ISA-95/Opc.ISA95.NodeSet2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/Machinery
	touch $LogsPath/Machinery/ServerImportNodeSet2XmlTest_Machinery_a2208e8.log
	output=$LogsPath/Machinery/ServerImportNodeSet2XmlTest_Machinery_a2208e8.log
fi
echo "... Machinery ..." >> $output
echo "Opc.Ua.Machinery.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.NodeSet2.xml >> $output
echo >> $output
echo "Opc.Ua.Machinery.Examples.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.Examples.NodeSet2.xml  >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/MachineTool
	touch $LogsPath/MachineTool/ServerImportNodeSet2XmlTest_MachineTool_a2208e8.log
	output=$LogsPath/MachineTool/ServerImportNodeSet2XmlTest_MachineTool_a2208e8.log
fi
echo "... MachineTool ..." >> $output
echo "Opc.Ua.MachineTool.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
						   $NodesetsBasePath/IA/Opc.Ua.IA.NodeSet2.xml \
						   $NodesetsBasePath/MachineTool/Opc.Ua.MachineTool.NodeSet2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/MachineVision
	touch $LogsPath/MachineVision/ServerImportNodeSet2XmlTest_MachineVision_a2208e8.log
	output=$LogsPath/MachineVision/ServerImportNodeSet2XmlTest_MachineVision_a2208e8.log
fi
echo "... MachineVision ..." >> $output
echo "Opc.Ua.MachineVision.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/MachineVision/Opc.Ua.MachineVision.NodeSet2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/MDIS
	touch $LogsPath/MDIS/ServerImportNodeSet2XmlTest_MDIS_a2208e8.log
	output=$LogsPath/MDIS/ServerImportNodeSet2XmlTest_MDIS_a2208e8.log
fi
echo "... MDIS ..." >> $output
echo "Opc.Ua.MDIS.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/MDIS/Opc.MDIS.NodeSet2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/Mining/DevelopmentSupport
	touch $LogsPath/Mining/DevelopmentSupport/ServerImportNodeSet2XmlTest_Mining-DevelopmentSupport_a2208e8.log
	output=$LogsPath/Mining/DevelopmentSupport/ServerImportNodeSet2XmlTest_Mining-DevelopmentSupport_a2208e8.log
fi
echo "... Mining ..." >> $output
echo ".. DevelopmentSupport .. " >> $output
echo "1.0.0/Opc.Ua.Mining.DevelopmentSupport.Dozer.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
						   $NodesetsBasePath/Mining/General/1.0.0/Opc.Ua.Mining.General.NodeSet2.xml \
						   $NodesetsBasePath/Mining/DevelopmentSupport/Dozer/1.0.0/Opc.Ua.Mining.DevelopmentSupport.Dozer.NodeSet2.xml >> $output
echo >> $output
echo "1.0.0/Opc.Ua.Mining.DevelopmentSupport.General.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/Mining/DevelopmentSupport/General/1.0.0/Opc.Ua.Mining.DevelopmentSupport.General.NodeSet2.xml >> $output
echo >> $output
echo "1.0.0/Opc.Ua.Mining.DevelopmentSupport.RoofSupportSystem.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
						   $NodesetsBasePath/Mining/General/1.0.0/Opc.Ua.Mining.General.NodeSet2.xml \
						   $NodesetsBasePath/Mining/DevelopmentSupport/RoofSupportSystem/1.0.0/Opc.Ua.Mining.DevelopmentSupport.RoofSupportSystem.NodeSet2.xml >> $output
echo >> $output
if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/Mining/Extraction
	touch $LogsPath/Mining/Extraction/ServerImportNodeSet2XmlTest_Mining-Extraction_a2208e8.log
	output=$LogsPath/Mining/Extraction/ServerImportNodeSet2XmlTest_Mining-Extraction_a2208e8.log
fi
echo ".. Extraction .. " >> $output
echo "1.0.0/Opc.Ua.Mining.Extraction.General.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/Mining/Extraction/General/1.0.0/Opc.Ua.Mining.Extraction.General.NodeSet2.xml >> $output
echo >> $output
echo "1.0.0/Opc.Ua.Mining.Extraction.ShearerLoader.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
						   $NodesetsBasePath/Mining/General/1.0.0/Opc.Ua.Mining.General.NodeSet2.xml \
						   $NodesetsBasePath/Mining/Extraction/ShearerLoader/1.0.0/Opc.Ua.Mining.Extraction.ShearerLoader.NodeSet2.xml >> $output
echo >> $output
if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/Mining/General
	touch $LogsPath/Mining/General/ServerImportNodeSet2XmlTest_Mining-General_a2208e8.log
	output=$LogsPath/Mining/General/ServerImportNodeSet2XmlTest_Mining-General_a2208e8.log
fi
echo ".. General .. " >> $output
echo "1.0.0/Opc.Ua.Mining.General.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
						   $NodesetsBasePath/Mining/General/1.0.0/Opc.Ua.Mining.General.NodeSet2.xml >> $output
echo >> $output
if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/Mining/Loading
	touch $LogsPath/Mining/Loading/ServerImportNodeSet2XmlTest_Mining-Loading_a2208e8.log
	output=$LogsPath/Mining/Loading/ServerImportNodeSet2XmlTest_Mining-Loading_a2208e8.log
fi
echo ".. Loading .. " >> $output
echo "1.0.0/Opc.Ua.Mining.Loading.General.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
						   $NodesetsBasePath/Mining/General/1.0.0/Opc.Ua.Mining.General.NodeSet2.xml \
						   $NodesetsBasePath/Mining/Loading/General/1.0.0/Opc.Ua.Mining.Loading.General.NodeSet2.xml >> $output
echo >> $output
echo "1.0.0/Opc.Ua.Mining.Loading.HydraulicExcavator.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
						   $NodesetsBasePath/Mining/General/1.0.0/Opc.Ua.Mining.General.NodeSet2.xml \
						   $NodesetsBasePath/Mining/Loading/General/1.0.0/Opc.Ua.Mining.Loading.General.NodeSet2.xml \
						   $NodesetsBasePath/Mining/Loading/HydraulicExcavator/1.0.0/Opc.Ua.Mining.Loading.HydraulicExcavator.NodeSet2.xml >> $output
echo >> $output
if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/Mining/MineralProcessing
	touch $LogsPath/Mining/MineralProcessing/ServerImportNodeSet2XmlTest_Mining-MineralProcessing_a2208e8.log
	output=$LogsPath/Mining/MineralProcessing/ServerImportNodeSet2XmlTest_Mining-MineralProcessing_a2208e8.log
fi
echo ".. MineralProcessing .. " >> $output
echo "1.0.0/Opc.Ua.Mining.MineralProcessing.General.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/Mining/MineralProcessing/General/1.0.0/Opc.Ua.Mining.MineralProcessing.General.NodeSet2.xml >> $output
echo >> $output
echo "1.0.0/Opc.Ua.Mining.MineralProcessing.RockCrusher.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
						   $NodesetsBasePath/Mining/General/1.0.0/Opc.Ua.Mining.General.NodeSet2.xml \
						   $NodesetsBasePath/Mining/MineralProcessing/RockCrusher/1.0.0/Opc.Ua.Mining.MineralProcessing.RockCrusher.NodeSet2.xml >> $output
echo >> $output
if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/Mining/MonitoringSupervisionServices
	touch $LogsPath/Mining/MonitoringSupervisionServices/ServerImportNodeSet2XmlTest_Mining-MonitoringSupervisionServices_a2208e8.log
	output=$LogsPath/Mining/MonitoringSupervisionServices/ServerImportNodeSet2XmlTest_Mining-MonitoringSupervisionServices_a2208e8.log
fi
echo ".. MonitoringSupervisionServices .. " >> $output
echo "1.0.0/Opc.Ua.Mining.MonitoringSupervisionServices.General.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/Mining/MonitoringSupervisionServices/General/1.0.0/Opc.Ua.Mining.MonitoringSupervisionServices.General.NodeSet2.xml >> $output
echo >> $output
if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/Mining/PELOServices
	touch $LogsPath/Mining/PELOServices/ServerImportNodeSet2XmlTest_Mining-PELOServices_a2208e8.log
	output=$LogsPath/Mining/PELOServices/ServerImportNodeSet2XmlTest_Mining-PELOServices_a2208e8.log
fi
echo ".. PELOServices .. " >> $output
echo "1.0.0/Opc.Ua.Mining.PELOServices.FaceAlignmentSystem.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
						   $NodesetsBasePath/Mining/General/1.0.0/Opc.Ua.Mining.General.NodeSet2.xml \
						   $NodesetsBasePath/Mining/PELOServices/FaceAlignmentSystem/1.0.0/Opc.Ua.Mining.PELOServices.FaceAlignmentSystem.NodeSet2.xml >> $output
echo >> $output
echo "1.0.0/Opc.Ua.Mining.PELOServices.General.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
						   $NodesetsBasePath/Mining/General/1.0.0/Opc.Ua.Mining.General.NodeSet2.xml \
						   $NodesetsBasePath/Mining/PELOServices/General/1.0.0/Opc.Ua.Mining.PELOServices.General.NodeSet2.xml >> $output
echo >> $output
if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/Mining/TransportDumping
	touch $LogsPath/Mining/TransportDumping/ServerImportNodeSet2XmlTest_Mining-TransportDumping_a2208e8.log
	output=$LogsPath/Mining/TransportDumping/ServerImportNodeSet2XmlTest_Mining-TransportDumping_a2208e8.log
fi
echo ".. TransportDumping .. " >> $output
echo "1.0.0/Opc.Ua.Mining.TransportDumping.ArmouredFaceConveyor.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
						   $NodesetsBasePath/Mining/General/1.0.0/Opc.Ua.Mining.General.NodeSet2.xml \
						   $NodesetsBasePath/Mining/TransportDumping/ArmouredFaceConveyor/1.0.0/Opc.Ua.Mining.TransportDumping.ArmouredFaceConveyor.NodeSet2.xml >> $output
echo >> $output
echo "1.0.0/Opc.Ua.Mining.TransportDumping.General.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
						   $NodesetsBasePath/Mining/General/1.0.0/Opc.Ua.Mining.General.NodeSet2.xml \
						   $NodesetsBasePath/Mining/TransportDumping/General/1.0.0/Opc.Ua.Mining.TransportDumping.General.NodeSet2.xml >> $output
echo >> $output
echo "1.0.0/Opc.Ua.Mining.TransportDumping.RearDumpTruck.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
						   $NodesetsBasePath/Mining/General/1.0.0/Opc.Ua.Mining.General.NodeSet2.xml \
						   $NodesetsBasePath/Mining/TransportDumping/General/1.0.0/Opc.Ua.Mining.TransportDumping.General.NodeSet2.xml \
						   $NodesetsBasePath/Mining/TransportDumping/RearDumpTruck/1.0.0/Opc.Ua.Mining.TransportDumping.RearDumpTruck.NodeSet2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/MTConnect
	touch $LogsPath/MTConnect/ServerImportNodeSet2XmlTest_MTConnect_a2208e8.log
	output=$LogsPath/MTConnect/ServerImportNodeSet2XmlTest_MTConnect_a2208e8.log
fi
echo "... MTConnect ..." >> $output
echo "Opc.Ua.MTConnect.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/MTConnect/Opc.Ua.MTConnect.NodeSet2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/OpenSCS
	touch $LogsPath/OpenSCS/ServerImportNodeSet2XmlTest_OpenSCS_a2208e8.log
	output=$LogsPath/OpenSCS/ServerImportNodeSet2XmlTest_OpenSCS_a2208e8.log
fi
echo "... OpenSCS ..." >> $output
echo "Opc.Ua.OPENSCS.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/OpenSCS/Opc.Ua.OPENSCS.NodeSet2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/PackML
	touch $LogsPath/PackML/ServerImportNodeSet2XmlTest_PackML_a2208e8.log
	output=$LogsPath/PackML/ServerImportNodeSet2XmlTest_PackML_a2208e8.log
fi
echo "... PackML ..." >> $output
echo "Opc.Ua.PackML.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/PackML/Opc.Ua.PackML.NodeSet2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/PADIM
	touch $LogsPath/PADIM/ServerImportNodeSet2XmlTest_PADIM_a2208e8.log
	output=$LogsPath/PADIM/ServerImportNodeSet2XmlTest_PADIM_a2208e8.log
fi
echo "... PADIM ..." >> $output
echo "Opc.Ua.IRDI.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/PADIM/Opc.Ua.IRDI.NodeSet2.xml >> $output
echo >> $output
echo "Opc.Ua.PADIM.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/PADIM/Opc.Ua.IRDI.NodeSet2.xml \
						   $NodesetsBasePath/PADIM/Opc.Ua.PADIM.NodeSet2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/PlasticsRubber/Extrusion
	touch $LogsPath/PlasticsRubber/Extrusion/ServerImportNodeSet2XmlTest_PlasticsRubber-Extrusion_a2208e8.log
	output=$LogsPath/PlasticsRubber/Extrusion/ServerImportNodeSet2XmlTest_PlasticsRubber-Extrusion_a2208e8.log
fi
echo "... PlasticsRubber ..." >> $output
echo ".. Extrusion .. " >> $output
echo "1.00/Opc.Ua.PlasticsRubber.Extrusion.Calender.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion/GeneralTypes/1.01/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion/Calender/1.00/Opc.Ua.PlasticsRubber.Extrusion.Calender.NodeSet2.xml >> $output
echo >> $output
echo "1.00/Opc.Ua.PlasticsRubber.Extrusion.Calibrator.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion/GeneralTypes/1.00/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion/Calibrator/1.00/Opc.Ua.PlasticsRubber.Extrusion.Calibrator.NodeSet2.xml >> $output
echo >> $output
echo "1.00/Opc.Ua.PlasticsRubber.Extrusion.Corrugator.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion/GeneralTypes/1.00/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion/HaulOff/1.00/Opc.Ua.PlasticsRubber.Extrusion.HaulOff.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion/Corrugator/1.00/Opc.Ua.PlasticsRubber.Extrusion.Corrugator.NodeSet2.xml >> $output
echo >> $output
echo "1.00/Opc.Ua.PlasticsRubber.Extrusion.Cutter.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion/GeneralTypes/1.00/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion/Cutter/1.00/Opc.Ua.PlasticsRubber.Extrusion.Cutter.NodeSet2.xml >> $output
echo >> $output
echo "1.00/Opc.Ua.PlasticsRubber.Extrusion.Die.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion/GeneralTypes/1.00/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion/Die/1.00/Opc.Ua.PlasticsRubber.Extrusion.Die.NodeSet2.xml >> $output
echo >> $output
echo "1.00/Opc.Ua.PlasticsRubber.Extrusion.Extruder.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion/GeneralTypes/1.00/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion/Extruder/1.00/Opc.Ua.PlasticsRubber.Extrusion.Extruder.NodeSet2.xml >> $output
echo >> $output
echo "1.00/Opc.Ua.PlasticsRubber.Extrusion.ExtrusionLine.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion/ExtrusionLine/1.00/Opc.Ua.PlasticsRubber.Extrusion.ExtrusionLine.NodeSet2.xml >> $output
echo >> $output
echo "1.00/Opc.Ua.PlasticsRubber.Extrusion.Filter.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion/GeneralTypes/1.00/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion/Filter/1.00/Opc.Ua.PlasticsRubber.Extrusion.Filter.NodeSet2.xml >> $output
echo >> $output
echo "1.00/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion/GeneralTypes/1.00/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml >> $output
echo >> $output
echo "1.01/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion/GeneralTypes/1.01/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml >> $output
echo >> $output
echo "1.00/Opc.Ua.PlasticsRubber.Extrusion.HaulOff.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion/GeneralTypes/1.00/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion/HaulOff/1.00/Opc.Ua.PlasticsRubber.Extrusion.HaulOff.NodeSet2.xml >> $output
echo >> $output
echo "1.00/Opc.Ua.PlasticsRubber.Extrusion.MeltPump.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion/GeneralTypes/1.00/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion/MeltPump/1.00/Opc.Ua.PlasticsRubber.Extrusion.MeltPump.NodeSet2.xml >> $output
echo >> $output
echo "1.00/Opc.Ua.PlasticsRubber.Extrusion.Pelletizer.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion/GeneralTypes/1.00/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion/Pelletizer/1.00/Opc.Ua.PlasticsRubber.Extrusion.Pelletizer.NodeSet2.xml >> $output
echo >> $output
if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/PlasticsRubber/Extrusion_v2
	touch $LogsPath/PlasticsRubber/Extrusion_v2/ServerImportNodeSet2XmlTest_PlasticsRubber-Extrusion_v2_a2208e8.log
	output=$LogsPath/PlasticsRubber/Extrusion_v2/ServerImportNodeSet2XmlTest_PlasticsRubber-Extrusion_v2_a2208e8.log
fi
echo ".. Extrusion_v2 .. " >> $output
echo "2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Calender.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion_v2/Calender/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Calender.NodeSet2.xml >> $output
echo >> $output
echo "2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Calibrator.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion_v2/Calibrator/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Calibrator.NodeSet2.xml >> $output
echo >> $output
echo "2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Corrugator.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion_v2/HaulOff/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.HaulOff.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion_v2/Corrugator/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Corrugator.NodeSet2.xml >> $output
echo >> $output
echo "2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Cutter.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion_v2/Cutter/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Cutter.NodeSet2.xml >> $output
echo >> $output
echo "2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Die.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion_v2/Die/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Die.NodeSet2.xml >> $output
echo >> $output
echo "2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Extruder.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion_v2/Extruder/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Extruder.NodeSet2.xml >> $output
echo >> $output
echo "2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.ExtrusionLine.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion_v2/ExtrusionLine/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.ExtrusionLine.NodeSet2.xml >> $output
echo >> $output
echo "2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Filter.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion_v2/Filter/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Filter.NodeSet2.xml >> $output
echo >> $output
echo "2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml >> $output
echo >> $output
echo "2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.HaulOff.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion_v2/HaulOff/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.HaulOff.NodeSet2.xml >> $output
echo >> $output
echo "2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.MeltPump.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion_v2/MeltPump/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.MeltPump.NodeSet2.xml >> $output
echo >> $output
echo "2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Pelletizer.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/Extrusion_v2/Pelletizer/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Pelletizer.NodeSet2.xml >> $output
echo >> $output
if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/PlasticsRubber/GeneralTypes
	touch $LogsPath/PlasticsRubber/GeneralTypes/ServerImportNodeSet2XmlTest_PlasticsRubber-GeneralTypes_a2208e8.log
	output=$LogsPath/PlasticsRubber/GeneralTypes/ServerImportNodeSet2XmlTest_PlasticsRubber-GeneralTypes_a2208e8.log
fi
echo ".. GeneralTypes .. " >> $output
echo "1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml >> $output
echo >> $output
echo "1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml >> $output
echo >> $output
if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/PlasticsRubber/HotRunner
	touch $LogsPath/PlasticsRubber/HotRunner/ServerImportNodeSet2XmlTest_PlasticsRubber-HotRunner_a2208e8.log
	output=$LogsPath/PlasticsRubber/HotRunner/ServerImportNodeSet2XmlTest_PlasticsRubber-HotRunner_a2208e8.log
fi
echo ".. HotRunner .. " >> $output
echo "1.00/Opc.Ua.PlasticsRubber.HotRunner.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/HotRunner/1.00/Opc.Ua.PlasticsRubber.HotRunner.NodeSet2.xml >> $output
echo >> $output
if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/PlasticsRubber/IMM2MES
	touch $LogsPath/PlasticsRubber/IMM2MES/ServerImportNodeSet2XmlTest_PlasticsRubber-IMM2MES_a2208e8.log
	output=$LogsPath/PlasticsRubber/IMM2MES/ServerImportNodeSet2XmlTest_PlasticsRubber-IMM2MES_a2208e8.log
fi
echo ".. IMM2MES .. " >> $output
echo "1.01/Opc.Ua.PlasticsRubber.IMM2MES.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/IMM2MES/1.01/Opc.Ua.PlasticsRubber.IMM2MES.NodeSet2.xml >> $output
echo >> $output
if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/PlasticsRubber/LDS
	touch $LogsPath/PlasticsRubber/LDS/ServerImportNodeSet2XmlTest_PlasticsRubber-LDS_a2208e8.log
	output=$LogsPath/PlasticsRubber/LDS/ServerImportNodeSet2XmlTest_PlasticsRubber-LDS_a2208e8.log
fi
echo ".. LDS .. " >> $output
echo "1.00/Opc.Ua.PlasticsRubber.LDS.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/LDS/1.00/Opc.Ua.PlasticsRubber.LDS.NodeSet2.xml >> $output
echo >> $output
if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/PlasticsRubber/TCD
	touch $LogsPath/PlasticsRubber/TCD/ServerImportNodeSet2XmlTest_PlasticsRubber-TCD_a2208e8.log
	output=$LogsPath/PlasticsRubber/TCD/ServerImportNodeSet2XmlTest_PlasticsRubber-TCD_a2208e8.log
fi
echo ".. TCD .. " >> $output
echo "1.01/Opc.Ua.PlasticsRubber.TCD.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/GeneralTypes/1.02/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml \
						   $NodesetsBasePath/PlasticsRubber/TCD/1.01/Opc.Ua.PlasticsRubber.TCD.NodeSet2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/PLCopen
	touch $LogsPath/PLCopen/ServerImportNodeSet2XmlTest_PLCopen_a2208e8.log
	output=$LogsPath/PLCopen/ServerImportNodeSet2XmlTest_PLCopen_a2208e8.log
fi
echo "... PLCopen ..." >> $output
echo "Opc.Ua.PLCopen.NodeSet2_V1.02.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/PLCopen/Opc.Ua.PLCopen.NodeSet2_V1.02.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/PNEM
	touch $LogsPath/PNEM/ServerImportNodeSet2XmlTest_PNEM_a2208e8.log
	output=$LogsPath/PNEM/ServerImportNodeSet2XmlTest_PNEM_a2208e8.log
fi
echo "... PNEM ..." >> $output
echo "Opc.Ua.PnEm.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/PNEM/Opc.Ua.PnEm.NodeSet2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/PNRIO
	touch $LogsPath/PNRIO/ServerImportNodeSet2XmlTest_PNRIO_a2208e8.log
	output=$LogsPath/PNRIO/ServerImportNodeSet2XmlTest_PNRIO_a2208e8.log
fi
echo "... PNRIO ..." >> $output
echo "Opc.Ua.PnRio.Nodeset2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/PNRIO/Opc.Ua.PnRio.Nodeset2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/POWERLINK
	touch $LogsPath/POWERLINK/ServerImportNodeSet2XmlTest_POWERLINK_a2208e8.log
	output=$LogsPath/POWERLINK/ServerImportNodeSet2XmlTest_POWERLINK_a2208e8.log
fi
echo "... POWERLINK ..." >> $output
echo "Opc.Ua.POWERLINK.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/POWERLINK/Opc.Ua.POWERLINK.NodeSet2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/PROFINET
	touch $LogsPath/PROFINET/ServerImportNodeSet2XmlTest_PROFINET_a2208e8.log
	output=$LogsPath/PROFINET/ServerImportNodeSet2XmlTest_PROFINET_a2208e8.log
fi
echo "... PROFINET ..." >> $output
echo "Opc.Ua.Pn.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/PROFINET/Opc.Ua.Pn.NodeSet2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/Pumps
	touch $LogsPath/Pumps/ServerImportNodeSet2XmlTest_Pumps_a2208e8.log
	output=$LogsPath/Pumps/ServerImportNodeSet2XmlTest_Pumps_a2208e8.log
fi
echo "... Pumps ..." >> $output
echo "Opc.Ua.Pumps.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
						   $NodesetsBasePath/Pumps/Opc.Ua.Pumps.NodeSet2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/Robotics
	touch $LogsPath/Robotics/ServerImportNodeSet2XmlTest_Robotics_a2208e8.log
	output=$LogsPath/Robotics/ServerImportNodeSet2XmlTest_Robotics_a2208e8.log
fi
echo "... Robotics ..." >> $output
echo "Opc.Ua.Robotics.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Robotics/Opc.Ua.Robotics.NodeSet2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/Safety
	touch $LogsPath/Safety/ServerImportNodeSet2XmlTest_Safety_a2208e8.log
	output=$LogsPath/Safety/ServerImportNodeSet2XmlTest_Safety_a2208e8.log
fi
echo "... Safety ..." >> $output
echo "Opc.Ua.Safety.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/Safety/Opc.Ua.Safety.NodeSet2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/Scales
	touch $LogsPath/Scales/ServerImportNodeSet2XmlTest_Scales_a2208e8.log
	output=$LogsPath/Scales/ServerImportNodeSet2XmlTest_Scales_a2208e8.log
fi
echo "... Scales ..." >> $output
echo "Opc.Ua.Scales.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Scales/Opc.Ua.Scales.NodeSet2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/Sercos
	touch $LogsPath/Sercos/ServerImportNodeSet2XmlTest_Sercos_a2208e8.log
	output=$LogsPath/Sercos/ServerImportNodeSet2XmlTest_Sercos_a2208e8.log
fi
echo "... Sercos ..." >> $output
echo "Sercos.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Sercos/Sercos.NodeSet2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/Weihenstephan
	touch $LogsPath/Weihenstephan/ServerImportNodeSet2XmlTest_Weihenstephan_a2208e8.log
	output=$LogsPath/Weihenstephan/ServerImportNodeSet2XmlTest_Weihenstephan_a2208e8.log
fi
echo "... Weihenstephan ..." >> $output
echo "Opc.Ua.Weihenstephan.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
						   $NodesetsBasePath/PackML/Opc.Ua.PackML.NodeSet2.xml \
						   $NodesetsBasePath/Weihenstephan/Opc.Ua.Weihenstephan.NodeSet2.xml >> $output
echo >> $output; echo >> $output

if [ "$1" = "1" ]; then
	mkdir -p $LogsPath/Woodworking
	touch $LogsPath/Woodworking/ServerImportNodeSet2XmlTest_Woodworking_a2208e8.log
	output=$LogsPath/Woodworking/ServerImportNodeSet2XmlTest_Woodworking_a2208e8.log
fi
echo "... Woodworking ..." >> $output
echo "Opc.Ua.Eumabois.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
						   $NodesetsBasePath/Woodworking/Opc.Ua.Woodworking.NodeSet2.xml \
						   $NodesetsBasePath/Woodworking/Opc.Ua.Eumabois.Nodeset2.xml >> $output
echo >> $output
echo "Opc.Ua.Woodworking.NodeSet2.xml" >> $output
$ExamplesBinPath/$TestAppName $NodesetsBasePath/DI/Opc.Ua.Di.NodeSet2.xml \
						   $NodesetsBasePath/Machinery/Opc.Ua.Machinery.NodeSet2.xml \
						   $NodesetsBasePath/Woodworking/Opc.Ua.Woodworking.NodeSet2.xml >> $output
echo >> $output; echo >> $output

echo "Press [Enter] key to continue..."

while : ; do
	read -n 1 -s k
	if [[ "$k" = "" ]] ; then
		break
	else
		echo "Press [Enter] key to continue..."
	fi
done