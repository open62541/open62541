foreach(MODEL ${UA_INFORMATION_MODEL_AUTOLOAD})

    string(TOLOWER "${MODEL}" NODESET_NAME)

    # ===================================================================
    # Level 0 — No dependency beyond the base OPC UA namespace
    # ===================================================================

    # NOTE: RSL and GPOS depend on OPC UA Part 8 spatial types (3DFrame,
    # 3DCartesianCoordinates) and use abstract VariableTypes as
    # TypeDefinitions, which are not yet fully supported.

    # Standard pattern: single XML, no companion-spec dependency.
    # Model folders that follow the Opc.Ua.<Name>.NodeSet2.xml naming.
    if(MODEL STREQUAL "DEXPI" OR MODEL STREQUAL "Safety" OR MODEL STREQUAL "CNC" OR MODEL STREQUAL "BACnet" OR
       MODEL STREQUAL "AMB" OR MODEL STREQUAL "MachineVision" OR MODEL STREQUAL "I4AAS" OR
       MODEL STREQUAL "PackML" OR MODEL STREQUAL "MTConnect" OR MODEL STREQUAL "Scheduler")

        # get csv file
        file(GLOB files "${UA_NODESET_DIR}/${MODEL}/*.csv")
        foreach(file ${files})
            string(REGEX MATCH "(.*nodeids.csv)|(.*NodeIds.csv)|(Opc.Ua.${MODEL}.NodeSet[2]?.csv)" csv_file_match ${file})
            if(csv_file_match)
                set(csv_file ${file})
            endif()
        endforeach()

        # get xml file
        file(GLOB files "${UA_NODESET_DIR}/${MODEL}/*.xml")
        foreach(file ${files})
            string(REGEX MATCH "Opc.Ua.${MODEL}.NodeSet[2]?.xml" xml_file_match ${file})
            if(xml_file_match)
                set(xml_file ${file})
            endif()
        endforeach()

        ua_generate_nodeset_and_datatypes(NAME "${NODESET_NAME}" FILE_CSV "${csv_file}"
                                          IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                                          FILE_NS "${xml_file}" INTERNAL AUTOLOAD)

    # ===================================================================
    # Level 1 — Depends on DI only
    # ===================================================================

    elseif(MODEL STREQUAL "CSPPlusForMachine" OR MODEL STREQUAL "POWERLINK" OR
           MODEL STREQUAL "IA" OR MODEL STREQUAL "FDT" OR MODEL STREQUAL "Robotics" OR
           MODEL STREQUAL "CommercialKitchenEquipment" OR MODEL STREQUAL "IOLink")

        # get csv file
        file(GLOB files "${UA_NODESET_DIR}/${MODEL}/*.csv")
        foreach(file ${files})
            string(REGEX MATCH "(.*nodeids.csv)|(.*NodeIds.csv)|(Opc.Ua.${MODEL}.NodeSet[2]?.csv)" csv_file_match ${file})
            if(csv_file_match)
                set(csv_file ${file})
            endif()
        endforeach()

        # get xml file
        file(GLOB files "${UA_NODESET_DIR}/${MODEL}/*.xml")
        foreach(file ${files})
            string(REGEX MATCH "Opc.Ua.${MODEL}.NodeSet[2]?.xml" xml_file_match ${file})
            if(xml_file_match)
                set(xml_file ${file})
            endif()
        endforeach()

        ua_generate_nodeset_and_datatypes(NAME "${NODESET_NAME}" FILE_CSV "${csv_file}"
                                          IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                                          FILE_NS "${xml_file}" INTERNAL AUTOLOAD DEPENDS "di")

    # ===================================================================
    # Level 2+ — Multiple / complex dependencies
    # ===================================================================

    # Machinery needs both DI and IA
    elseif(MODEL STREQUAL "Machinery")

        # get csv file
        file(GLOB files "${UA_NODESET_DIR}/${MODEL}/*.csv")
        foreach(file ${files})
            string(REGEX MATCH "(.*nodeids.csv)|(.*NodeIds.csv)|(Opc.Ua.${MODEL}.NodeSet[2]?.csv)" csv_file_match ${file})
            if(csv_file_match)
                set(csv_file ${file})
            endif()
        endforeach()

        # get xml file
        file(GLOB files "${UA_NODESET_DIR}/${MODEL}/*.xml")
        foreach(file ${files})
            string(REGEX MATCH "Opc.Ua.${MODEL}.NodeSet[2]?.xml" xml_file_match ${file})
            if(xml_file_match)
                set(xml_file ${file})
            endif()
        endforeach()

        ua_generate_nodeset_and_datatypes(NAME "${NODESET_NAME}" FILE_CSV "${csv_file}"
                                          IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                                          FILE_NS "${xml_file}" INTERNAL AUTOLOAD DEPENDS "di" DEPENDS "ia")

    elseif(MODEL STREQUAL "CAS")
        ua_generate_nodeset_and_datatypes(NAME "${NODESET_NAME}"
                                          FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeIds.csv"
                                          FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeSet2.xml"
                                          INTERNAL AUTOLOAD DEPENDS "di" DEPENDS "machinery" DEPENDS "ia")

    elseif(MODEL STREQUAL "TMC")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "packml")

    elseif(MODEL STREQUAL "Pumps")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeSet2.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "machinery")

    elseif(MODEL STREQUAL "Weihenstephan")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "machinery" DEPENDS "packml")

    elseif(MODEL STREQUAL "Machinery-Jobs")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/Machinery/Jobs/Opc.Ua.Machinery.Jobs.NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                IMPORT_BSD "TYPES_ISA95_JOBCONTROL#${UA_NODESET_DIR}/ISA95-JOBCONTROL/opc.ua.isa95-jobcontrol.types.bsd"
                FILE_NS "${UA_NODESET_DIR}/Machinery/Jobs/Opc.Ua.Machinery.Jobs.Nodeset2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "isa95-jobcontrol")

    elseif(MODEL STREQUAL "Machinery-Result")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/Machinery/Result/Opc.Ua.Machinery_Result.NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/Machinery/Result/Opc.Ua.Machinery_Result.NodeSet2.xml"
                INTERNAL AUTOLOAD)

    elseif(MODEL STREQUAL "Machinery-ProcessValues")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/Machinery/ProcessValues/Opc.Ua.Machinery.ProcessValues.NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/Machinery/ProcessValues/Opc.Ua.Machinery.ProcessValues.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "irdi" DEPENDS "padim")

    elseif(MODEL STREQUAL "MachineTool")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.MachineTool.NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeSet2.xml"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                IMPORT_BSD "TYPES_ISA95_JOBCONTROL#${UA_NODESET_DIR}/ISA95-JOBCONTROL/opc.ua.isa95-jobcontrol.types.bsd"
                IMPORT_BSD "TYPES_MACHINERY_JOBS#${UA_NODESET_DIR}/Machinery/Jobs/Opc.Ua.Machinery.Jobs.Types.bsd"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "machinery" DEPENDS "ia"
                DEPENDS "isa95-jobcontrol" DEPENDS "machinery-jobs")

    elseif(MODEL STREQUAL "Scales")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "machinery" DEPENDS "packml")

    elseif(MODEL STREQUAL "Woodworking")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "machinery"
                DEPENDS "isa95-jobcontrol" DEPENDS "machinery-jobs")

    elseif(MODEL STREQUAL "Eumabois")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/Woodworking/Opc.ua.Eumabois.NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/Woodworking/Opc.Ua.Eumabois.Nodeset2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "machinery" DEPENDS "woodworking")

    # NOTE: MachineVision and Machinery-Result both independently define types
    # with the same C name (ProcessingTimesDataType, ResultDataType). They
    # cannot be loaded simultaneously.

    elseif(MODEL STREQUAL "ECM")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "ia")

    elseif(MODEL STREQUAL "Machinery-Energy")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/Machinery/Energy/Opc.Ua.Machinery.Energy.NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                IMPORT_BSD "TYPES_ECM#${UA_NODESET_DIR}/ECM/Opc.Ua.ECM.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/Machinery/Energy/Opc.Ua.Machinery.Energy.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "ia" DEPENDS "ecm")

    elseif(MODEL STREQUAL "SurfaceTechnology")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/SurfaceTechnology/Plasma/Opc.Ua.SurfaceTechnology.Plasma.NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/SurfaceTechnology/Plasma/Opc.Ua.SurfaceTechnology.Plasma.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "ia" DEPENDS "machinery"
                DEPENDS "isa95-jobcontrol" DEPENDS "machinery-jobs")

    elseif(MODEL STREQUAL "STGeneralTypes")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/SurfaceTechnology/GeneralTypes/Opc.Ua.${MODEL}.NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/SurfaceTechnology/GeneralTypes/Opc.Ua.${MODEL}.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "ia" DEPENDS "machinery"
                DEPENDS "isa95-jobcontrol" DEPENDS "machinery-jobs")

    elseif(MODEL STREQUAL "Shotblasting")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "machinery"
                DEPENDS "isa95-jobcontrol" DEPENDS "machinery-jobs")

    elseif(MODEL STREQUAL "LaserSystems")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "machinery" DEPENDS "ia" DEPENDS "machinetool"
                DEPENDS "isa95-jobcontrol" DEPENDS "machinery-jobs")

    elseif(MODEL STREQUAL "Onboarding")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "gds")

    elseif(MODEL STREQUAL "PNENC")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.PnEnc.Nodeset2.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.PnEnc.Nodeset2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di")

    elseif(MODEL STREQUAL "PNDRV")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.PNDRV.NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.PNDRV.Nodeset2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "pnenc")

    elseif(MODEL STREQUAL "PNGSDGM")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/opc.ua.pngsdgm.NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/opc.ua.pngsdgm.Nodeset2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di")

    elseif(MODEL STREQUAL "WoT")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.WotCon.NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.WotCon.NodeSet2.xml"
                INTERNAL AUTOLOAD)

    # ===================================================================
    # Specs with non-standard file naming
    # ===================================================================

    elseif(MODEL STREQUAL "DI")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.Di.NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.Di.NodeSet2.xml"
                INTERNAL AUTOLOAD)

    elseif(MODEL STREQUAL "AutoID")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.AutoID.NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.AutoID.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di")

    elseif(MODEL STREQUAL "PROFINET")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.Pn.NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.Pn.NodeSet2.xml"
                INTERNAL AUTOLOAD)

    elseif(MODEL STREQUAL "Glass")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Flat/NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Flat/Opc.Ua.${MODEL}.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "machinery")

    elseif(MODEL STREQUAL "IJT")
        ua_generate_nodeset_and_datatypes(
                NAME "ijt-base"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Base/Opc.Ua.Ijt.Base.NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                IMPORT_BSD "TYPES_MACHINERY_RESULT#${UA_NODESET_DIR}/Machinery/Result/Opc.Ua.Machinery_Result.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Base/Opc.Ua.Ijt.Base.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "amb" DEPENDS "ia"
                DEPENDS "machinery" DEPENDS "machinery-result")

        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Tightening/Opc.Ua.Ijt.Tightening.NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                IMPORT_BSD "TYPES_MACHINERY_RESULT#${UA_NODESET_DIR}/Machinery/Result/Opc.Ua.Machinery_Result.Types.bsd"
                IMPORT_BSD "TYPES_IJT_BASE#${UA_NODESET_DIR}/IJT/Base/Opc.Ua.Ijt.Base.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Tightening/Opc.Ua.Ijt.Tightening.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "amb" DEPENDS "ia"
                DEPENDS "machinery" DEPENDS "machinery-result"
                DEPENDS "ijt-base")

    elseif(MODEL STREQUAL "OpenSCS")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.OPENSCS.NodeSet2.xml"
                INTERNAL AUTOLOAD)

    # NOTE: PNEM and ECM both independently define types with the same C name
    # (AcPeDataType, AcPpDataType, EnergyStateInformationDataType,
    # StandbyModeTransitionDataType). They cannot be loaded simultaneously.

    elseif(MODEL STREQUAL "PNEM")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.PnEm.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di")

    elseif(MODEL STREQUAL "PNRIO")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.PnRio.Nodeset2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di")

    elseif(MODEL STREQUAL "ISA95-JOBCONTROL")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/opc.ua.isa95-jobcontrol.nodeids.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/opc.ua.isa95-jobcontrol.nodeset2.xml"
                INTERNAL AUTOLOAD)

    elseif(MODEL STREQUAL "IREDES")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.IREDES.NodeIds.csv"
                FILE_BSD "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.IREDES.Types.bsd"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.IREDES.NodeSet2.xml"
                INTERNAL AUTOLOAD)

    elseif(MODEL STREQUAL "PLCopen")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeSet2_V1.02.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeSet2_V1.02.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di")

    elseif(MODEL STREQUAL "ADI")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/OpcUaAdiModel.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.Adi.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di")

    elseif(MODEL STREQUAL "MDIS")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.MDIS.NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.MDIS.NodeSet2.xml"
                INTERNAL AUTOLOAD)

    elseif(MODEL STREQUAL "Sercos")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/SercosModel.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Sercos.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di")

    elseif(MODEL STREQUAL "GDS")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/OpcUaGdsModel.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.Gds.NodeSet2.xml"
                INTERNAL AUTOLOAD)

    elseif(MODEL STREQUAL "PADIM")
        ua_generate_nodeset_and_datatypes(NAME "irdi"
                                          FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.IRDI.NodeSet2.xml"
                                          INTERNAL AUTOLOAD)

        set(TMP_NSURI "http://opcfoundation.org/UA/${MODEL}/")
        configure_file(${PROJECT_SOURCE_DIR}/tools/nodeset_injector/empty.bsd.template
                       "${CMAKE_BINARY_DIR}/bsd_files_gen/Opc.Ua.${MODEL}.Types.bsd")
        ua_generate_nodeset_and_datatypes(NAME "${NODESET_NAME}"
                                          FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeSet2.xml"
                                          FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeIds.csv"
                                          FILE_BSD "${CMAKE_BINARY_DIR}/bsd_files_gen/Opc.Ua.${MODEL}.Types.bsd"
                                          INTERNAL AUTOLOAD DEPENDS "di" DEPENDS "irdi")

    elseif(MODEL STREQUAL "LADS")
        ua_generate_nodeset_and_datatypes(NAME "${NODESET_NAME}"
                                          FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeIds.csv"
                                          FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeSet2.xml"
                                          INTERNAL AUTOLOAD DEPENDS "di" DEPENDS "amb" DEPENDS "machinery")

    elseif(MODEL STREQUAL "GMS")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/opc.ua.gms.nodesids.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/opc.ua.gms.nodeset2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "ia" DEPENDS "machinery"
                DEPENDS "machinetool" DEPENDS "machinery-result"
                DEPENDS "isa95-jobcontrol" DEPENDS "machinery-jobs")

    elseif(MODEL STREQUAL "TTD")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/opc.ua.ttd.nodeids.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                IMPORT_BSD "TYPES_MACHINERY_RESULT#${UA_NODESET_DIR}/Machinery/Result/Opc.Ua.Machinery_Result.Types.bsd"
                IMPORT_BSD "TYPES_ISA95_JOBCONTROL#${UA_NODESET_DIR}/ISA95-JOBCONTROL/opc.ua.isa95-jobcontrol.types.bsd"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/opc.ua.ttd.nodeset2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "ia" DEPENDS "machinery"
                DEPENDS "machinery-result" DEPENDS "machinetool"
                DEPENDS "isa95-jobcontrol" DEPENDS "machinery-jobs")

    elseif(MODEL STREQUAL "IOLinkIODD")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/IOLink/Opc.Ua.IOLinkIODD.NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/IOLink/Opc.Ua.IOLinkIODD.NodeSet2.xml"
                INTERNAL AUTOLOAD)

    elseif(MODEL STREQUAL "WireHarness-VEC")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/WireHarness/opc.ua.wireharness.vec.nodeids.csv"
                FILE_NS "${UA_NODESET_DIR}/WireHarness/opc.ua.wireharness.vec.nodeset2.xml"
                INTERNAL AUTOLOAD)

    elseif(MODEL STREQUAL "WireHarness")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/WireHarness/opc.ua.wireharness.nodeids.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                IMPORT_BSD "TYPES_WIREHARNESS_VEC#${UA_NODESET_DIR}/WireHarness/opc.ua.wireharness.vec.types.bsd"
                FILE_NS "${UA_NODESET_DIR}/WireHarness/opc.ua.wireharness.nodeset2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "machinery" DEPENDS "machinery-result"
                DEPENDS "isa95-jobcontrol" DEPENDS "machinery-jobs"
                DEPENDS "wireharness-vec")

    elseif(MODEL STREQUAL "CuttingTool")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "machinery" DEPENDS "ia"
                DEPENDS "machinery-result" DEPENDS "machinetool"
                DEPENDS "isa95-jobcontrol" DEPENDS "machinery-jobs"
                DEPENDS "gms")

    # ===================================================================
    # UAFX (Field eXchange) models
    # ===================================================================

    elseif(MODEL STREQUAL "UAFX-Data")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/UAFX/opc.ua.fx.data.nodeset2.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/UAFX/opc.ua.fx.data.nodeset2.xml"
                INTERNAL AUTOLOAD)

    elseif(MODEL STREQUAL "UAFX-AC")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/UAFX/opc.ua.fx.ac.nodeset2.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                IMPORT_BSD "TYPES_UAFX_DATA#${UA_NODESET_DIR}/UAFX/opc.ua.fx.data.nodeset2.bsd"
                FILE_NS "${UA_NODESET_DIR}/UAFX/opc.ua.fx.ac.nodeset2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "uafx-data")

    elseif(MODEL STREQUAL "UAFX-CM")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/UAFX/opc.ua.fx.cm.nodeset2.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                IMPORT_BSD "TYPES_UAFX_DATA#${UA_NODESET_DIR}/UAFX/opc.ua.fx.data.nodeset2.bsd"
                FILE_NS "${UA_NODESET_DIR}/UAFX/opc.ua.fx.cm.nodeset2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "uafx-data")

    # ===================================================================
    # PlasticsRubber models
    # ===================================================================

    elseif(MODEL STREQUAL "PlasticsRubber-GeneralTypes")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/PlasticsRubber/GeneralTypes/1.03/NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/PlasticsRubber/GeneralTypes/1.03/Opc.Ua.PlasticsRubber.GeneralTypes.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di")

    elseif(MODEL STREQUAL "PlasticsRubber-TCD")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/PlasticsRubber/TCD/1.01/NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/PlasticsRubber/TCD/1.01/Opc.Ua.PlasticsRubber.TCD.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "plasticsrubber-generaltypes")

    elseif(MODEL STREQUAL "PlasticsRubber-IMM2MES")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/PlasticsRubber/IMM2MES/1.01/NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/PlasticsRubber/IMM2MES/1.01/Opc.Ua.PlasticsRubber.IMM2MES.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "plasticsrubber-generaltypes")

    elseif(MODEL STREQUAL "PlasticsRubber-LDS")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/PlasticsRubber/LDS/1.02/Opc.Ua.PlasticsRubber.LDS.NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/PlasticsRubber/LDS/1.02/Opc.Ua.PlasticsRubber.LDS.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "plasticsrubber-generaltypes")

    elseif(MODEL STREQUAL "PlasticsRubber-HotRunner")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/PlasticsRubber/HotRunner/1.00/NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/PlasticsRubber/HotRunner/1.00/Opc.Ua.PlasticsRubber.HotRunner.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "plasticsrubber-generaltypes")

    # ===================================================================
    # PlasticsRubber Extrusion v1 models
    # ===================================================================

    elseif(MODEL STREQUAL "PlasticsRubber-Extrusion-GeneralTypes")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/PlasticsRubber/Extrusion/GeneralTypes/1.01/NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/PlasticsRubber/Extrusion/GeneralTypes/1.01/Opc.Ua.PlasticsRubber.Extrusion.GeneralTypes.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "plasticsrubber-generaltypes")

    elseif(MODEL STREQUAL "PlasticsRubber-Extrusion-ExtrusionLine")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/PlasticsRubber/Extrusion/ExtrusionLine/1.00/NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/PlasticsRubber/Extrusion/ExtrusionLine/1.00/Opc.Ua.PlasticsRubber.Extrusion.ExtrusionLine.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "plasticsrubber-generaltypes")

    elseif(MODEL STREQUAL "PlasticsRubber-Extrusion-Extruder")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/PlasticsRubber/Extrusion/Extruder/1.00/NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/PlasticsRubber/Extrusion/Extruder/1.00/Opc.Ua.PlasticsRubber.Extrusion.Extruder.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "plasticsrubber-generaltypes"
                DEPENDS "plasticsrubber-extrusion-generaltypes")

    elseif(MODEL STREQUAL "PlasticsRubber-Extrusion-Die")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/PlasticsRubber/Extrusion/Die/1.00/NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/PlasticsRubber/Extrusion/Die/1.00/Opc.Ua.PlasticsRubber.Extrusion.Die.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "plasticsrubber-generaltypes"
                DEPENDS "plasticsrubber-extrusion-generaltypes")

    elseif(MODEL STREQUAL "PlasticsRubber-Extrusion-MeltPump")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/PlasticsRubber/Extrusion/MeltPump/1.00/NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/PlasticsRubber/Extrusion/MeltPump/1.00/Opc.Ua.PlasticsRubber.Extrusion.MeltPump.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "plasticsrubber-generaltypes"
                DEPENDS "plasticsrubber-extrusion-generaltypes")

    elseif(MODEL STREQUAL "PlasticsRubber-Extrusion-Filter")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/PlasticsRubber/Extrusion/Filter/1.00/NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/PlasticsRubber/Extrusion/Filter/1.00/Opc.Ua.PlasticsRubber.Extrusion.Filter.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "plasticsrubber-generaltypes"
                DEPENDS "plasticsrubber-extrusion-generaltypes")

    elseif(MODEL STREQUAL "PlasticsRubber-Extrusion-HaulOff")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/PlasticsRubber/Extrusion/HaulOff/1.00/NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/PlasticsRubber/Extrusion/HaulOff/1.00/Opc.Ua.PlasticsRubber.Extrusion.HaulOff.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "plasticsrubber-generaltypes"
                DEPENDS "plasticsrubber-extrusion-generaltypes")

    elseif(MODEL STREQUAL "PlasticsRubber-Extrusion-Cutter")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/PlasticsRubber/Extrusion/Cutter/1.00/NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/PlasticsRubber/Extrusion/Cutter/1.00/Opc.Ua.PlasticsRubber.Extrusion.Cutter.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "plasticsrubber-generaltypes"
                DEPENDS "plasticsrubber-extrusion-generaltypes")

    elseif(MODEL STREQUAL "PlasticsRubber-Extrusion-Calibrator")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/PlasticsRubber/Extrusion/Calibrator/1.00/NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/PlasticsRubber/Extrusion/Calibrator/1.00/Opc.Ua.PlasticsRubber.Extrusion.Calibrator.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "plasticsrubber-generaltypes"
                DEPENDS "plasticsrubber-extrusion-generaltypes")

    elseif(MODEL STREQUAL "PlasticsRubber-Extrusion-Calender")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/PlasticsRubber/Extrusion/Calender/1.00/NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/PlasticsRubber/Extrusion/Calender/1.00/Opc.Ua.PlasticsRubber.Extrusion.Calender.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "plasticsrubber-generaltypes"
                DEPENDS "plasticsrubber-extrusion-generaltypes")

    elseif(MODEL STREQUAL "PlasticsRubber-Extrusion-Pelletizer")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/PlasticsRubber/Extrusion/Pelletizer/1.00/NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/PlasticsRubber/Extrusion/Pelletizer/1.00/Opc.Ua.PlasticsRubber.Extrusion.Pelletizer.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "plasticsrubber-generaltypes"
                DEPENDS "plasticsrubber-extrusion-generaltypes")

    elseif(MODEL STREQUAL "PlasticsRubber-Extrusion-Corrugator")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/PlasticsRubber/Extrusion/Corrugator/1.00/NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/PlasticsRubber/Extrusion/Corrugator/1.00/Opc.Ua.PlasticsRubber.Extrusion.Corrugator.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "plasticsrubber-generaltypes"
                DEPENDS "plasticsrubber-extrusion-generaltypes"
                DEPENDS "plasticsrubber-extrusion-hauloff")

    # ===================================================================
    # PlasticsRubber Extrusion v2 models (requires Machinery)
    # ===================================================================

    elseif(MODEL STREQUAL "PlasticsRubber-Extrusion_v2-GeneralTypes")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/PlasticsRubber/Extrusion_v2/GeneralTypes/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.GeneralTypes.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "ia" DEPENDS "machinery"
                DEPENDS "plasticsrubber-generaltypes")

    elseif(MODEL STREQUAL "PlasticsRubber-Extrusion_v2-ExtrusionLine")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/PlasticsRubber/Extrusion_v2/ExtrusionLine/2.00/NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/PlasticsRubber/Extrusion_v2/ExtrusionLine/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.ExtrusionLine.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "ia" DEPENDS "machinery"
                DEPENDS "plasticsrubber-generaltypes"
                DEPENDS "plasticsrubber-extrusion_v2-generaltypes")

    elseif(MODEL STREQUAL "PlasticsRubber-Extrusion_v2-Extruder")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/PlasticsRubber/Extrusion_v2/Extruder/2.00/NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/PlasticsRubber/Extrusion_v2/Extruder/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Extruder.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "ia" DEPENDS "machinery"
                DEPENDS "plasticsrubber-generaltypes"
                DEPENDS "plasticsrubber-extrusion_v2-generaltypes")

    elseif(MODEL STREQUAL "PlasticsRubber-Extrusion_v2-Die")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/PlasticsRubber/Extrusion_v2/Die/2.00/NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/PlasticsRubber/Extrusion_v2/Die/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Die.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "ia" DEPENDS "machinery"
                DEPENDS "plasticsrubber-generaltypes"
                DEPENDS "plasticsrubber-extrusion_v2-generaltypes")

    elseif(MODEL STREQUAL "PlasticsRubber-Extrusion_v2-MeltPump")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/PlasticsRubber/Extrusion_v2/MeltPump/2.00/NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/PlasticsRubber/Extrusion_v2/MeltPump/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.MeltPump.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "ia" DEPENDS "machinery"
                DEPENDS "plasticsrubber-generaltypes"
                DEPENDS "plasticsrubber-extrusion_v2-generaltypes")

    elseif(MODEL STREQUAL "PlasticsRubber-Extrusion_v2-Filter")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/PlasticsRubber/Extrusion_v2/Filter/2.00/NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/PlasticsRubber/Extrusion_v2/Filter/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Filter.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "ia" DEPENDS "machinery"
                DEPENDS "plasticsrubber-generaltypes"
                DEPENDS "plasticsrubber-extrusion_v2-generaltypes")

    elseif(MODEL STREQUAL "PlasticsRubber-Extrusion_v2-HaulOff")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/PlasticsRubber/Extrusion_v2/HaulOff/2.00/NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/PlasticsRubber/Extrusion_v2/HaulOff/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.HaulOff.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "ia" DEPENDS "machinery"
                DEPENDS "plasticsrubber-generaltypes"
                DEPENDS "plasticsrubber-extrusion_v2-generaltypes")

    elseif(MODEL STREQUAL "PlasticsRubber-Extrusion_v2-Cutter")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/PlasticsRubber/Extrusion_v2/Cutter/2.00/NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/PlasticsRubber/Extrusion_v2/Cutter/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Cutter.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "ia" DEPENDS "machinery"
                DEPENDS "plasticsrubber-generaltypes"
                DEPENDS "plasticsrubber-extrusion_v2-generaltypes")

    elseif(MODEL STREQUAL "PlasticsRubber-Extrusion_v2-Calibrator")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/PlasticsRubber/Extrusion_v2/Calibrator/2.00/NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/PlasticsRubber/Extrusion_v2/Calibrator/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Calibrator.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "ia" DEPENDS "machinery"
                DEPENDS "plasticsrubber-generaltypes"
                DEPENDS "plasticsrubber-extrusion_v2-generaltypes")

    elseif(MODEL STREQUAL "PlasticsRubber-Extrusion_v2-Calender")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/PlasticsRubber/Extrusion_v2/Calender/2.00/NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/PlasticsRubber/Extrusion_v2/Calender/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Calender.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "ia" DEPENDS "machinery"
                DEPENDS "plasticsrubber-generaltypes"
                DEPENDS "plasticsrubber-extrusion_v2-generaltypes")

    elseif(MODEL STREQUAL "PlasticsRubber-Extrusion_v2-Pelletizer")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/PlasticsRubber/Extrusion_v2/Pelletizer/2.00/NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/PlasticsRubber/Extrusion_v2/Pelletizer/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Pelletizer.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "ia" DEPENDS "machinery"
                DEPENDS "plasticsrubber-generaltypes"
                DEPENDS "plasticsrubber-extrusion_v2-generaltypes")

    elseif(MODEL STREQUAL "PlasticsRubber-Extrusion_v2-Corrugator")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/PlasticsRubber/Extrusion_v2/Corrugator/2.00/NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/PlasticsRubber/Extrusion_v2/Corrugator/2.00/Opc.Ua.PlasticsRubber.Extrusion_v2.Corrugator.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "ia" DEPENDS "machinery"
                DEPENDS "plasticsrubber-generaltypes"
                DEPENDS "plasticsrubber-extrusion_v2-generaltypes"
                DEPENDS "plasticsrubber-extrusion_v2-hauloff")

    # ===================================================================
    # Additional standard companion specs
    # ===================================================================

    elseif(MODEL STREQUAL "AdditiveManufacturing")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                IMPORT_BSD "TYPES_MACHINERY_RESULT#${UA_NODESET_DIR}/Machinery/Result/Opc.Ua.Machinery_Result.Types.bsd"
                IMPORT_BSD "TYPES_ISA95_JOBCONTROL#${UA_NODESET_DIR}/ISA95-JOBCONTROL/opc.ua.isa95-jobcontrol.types.bsd"
                IMPORT_BSD "TYPES_MACHINERY_JOBS#${UA_NODESET_DIR}/Machinery/Jobs/Opc.Ua.Machinery.Jobs.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.Nodeset2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "ia" DEPENDS "machinery"
                DEPENDS "irdi" DEPENDS "padim" DEPENDS "machinery-processvalues"
                DEPENDS "isa95-jobcontrol" DEPENDS "machinery-jobs"
                DEPENDS "machinetool")

    elseif(MODEL STREQUAL "Powertrain")
        # Powertrain ships an additional IRDI dictionary file with IEC 62683
        # entries that are not part of the standard IRDI nodeset (IEC 61987).
        ua_generate_nodeset_and_datatypes(NAME "irdi-powertrain"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/powertraindictionary.nodeset2.xml"
                INTERNAL AUTOLOAD DEPENDS "irdi")

        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeIds.csv"
                FILE_BSD "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.Types.bsd"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "machinery"
                DEPENDS "uafx-data" DEPENDS "uafx-ac"
                DEPENDS "irdi-powertrain")

    elseif(MODEL STREQUAL "MetalForming")
        # PADIM 1.02.0 removed IRDI entries still referenced by MetalForming.
        # Load a small supplement with the missing entries before MetalForming.
        ua_generate_nodeset_and_datatypes(
                NAME "irdi-metalforming"
                FILE_NS "${PROJECT_SOURCE_DIR}/tools/nodeset_injector/schema/metalforming_irdi_supplement.xml"
                INTERNAL AUTOLOAD DEPENDS "irdi")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                IMPORT_BSD "TYPES_MACHINERY_RESULT#${UA_NODESET_DIR}/Machinery/Result/Opc.Ua.Machinery_Result.Types.bsd"
                IMPORT_BSD "TYPES_ISA95_JOBCONTROL#${UA_NODESET_DIR}/ISA95-JOBCONTROL/opc.ua.isa95-jobcontrol.types.bsd"
                IMPORT_BSD "TYPES_MACHINERY_JOBS#${UA_NODESET_DIR}/Machinery/Jobs/Opc.Ua.Machinery.Jobs.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "ia" DEPENDS "machinery"
                DEPENDS "irdi" DEPENDS "irdi-metalforming" DEPENDS "padim" DEPENDS "machinery-processvalues"
                DEPENDS "machinetool"
                DEPENDS "isa95-jobcontrol" DEPENDS "machinery-jobs")

    elseif(MODEL STREQUAL "PAEFS")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "machinery"
                DEPENDS "irdi" DEPENDS "padim" DEPENDS "machinery-processvalues")

    elseif(MODEL STREQUAL "WMTP")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.Nodeset2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "machinery"
                DEPENDS "irdi" DEPENDS "padim" DEPENDS "machinery-processvalues")

    # ===================================================================
    # FDI (Field Device Integration) models
    # ===================================================================

    elseif(MODEL STREQUAL "FDI5")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/FDI/OpcUaFdiPart5Model.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/FDI/Opc.Ua.Fdi5.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di")

    elseif(MODEL STREQUAL "FDI7")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/FDI/OpcUaFdiPart7Model.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/FDI/Opc.Ua.Fdi7.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di")

    # ===================================================================
    # Mining models
    # ===================================================================

    elseif(MODEL STREQUAL "Mining-General")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/Mining/General/1.0.0/NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/Mining/General/1.0.0/Opc.Ua.Mining.General.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "ia" DEPENDS "machinery")

    # Mining Extraction
    elseif(MODEL STREQUAL "Mining-Extraction-General")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/Mining/Extraction/General/1.0.0/NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/Mining/Extraction/General/1.0.0/Opc.Ua.Mining.Extraction.General.NodeSet2.xml"
                INTERNAL AUTOLOAD)

    elseif(MODEL STREQUAL "Mining-Extraction-ShearerLoader")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/Mining/Extraction/ShearerLoader/1.0.0/NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/Mining/Extraction/ShearerLoader/1.0.0/Opc.Ua.Mining.Extraction.ShearerLoader.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "machinery" DEPENDS "mining-general")

    # Mining Loading
    elseif(MODEL STREQUAL "Mining-Loading-General")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/Mining/Loading/General/1.0.0/NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/Mining/Loading/General/1.0.0/Opc.Ua.Mining.Loading.General.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "machinery" DEPENDS "mining-general")

    elseif(MODEL STREQUAL "Mining-Loading-HydraulicExcavator")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/Mining/Loading/HydraulicExcavator/1.0.0/NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/Mining/Loading/HydraulicExcavator/1.0.0/Opc.Ua.Mining.Loading.HydraulicExcavator.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "machinery" DEPENDS "mining-general"
                DEPENDS "mining-loading-general")

    # Mining DevelopmentSupport
    elseif(MODEL STREQUAL "Mining-DevelopmentSupport-General")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/Mining/DevelopmentSupport/General/1.0.0/NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/Mining/DevelopmentSupport/General/1.0.0/Opc.Ua.Mining.DevelopmentSupport.General.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "machinery" DEPENDS "mining-general")

    elseif(MODEL STREQUAL "Mining-DevelopmentSupport-RoofSupportSystem")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/Mining/DevelopmentSupport/RoofSupportSystem/1.0.0/NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/Mining/DevelopmentSupport/RoofSupportSystem/1.0.0/Opc.Ua.Mining.DevelopmentSupport.RoofSupportSystem.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "machinery" DEPENDS "mining-general"
                DEPENDS "mining-developmentsupport-general")

    elseif(MODEL STREQUAL "Mining-DevelopmentSupport-Dozer")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/Mining/DevelopmentSupport/Dozer/1.0.0/NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/Mining/DevelopmentSupport/Dozer/1.0.0/Opc.Ua.Mining.DevelopmentSupport.Dozer.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "machinery" DEPENDS "mining-general"
                DEPENDS "mining-developmentsupport-general")

    # Mining TransportDumping
    elseif(MODEL STREQUAL "Mining-TransportDumping-General")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/Mining/TransportDumping/General/1.0.0/NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/Mining/TransportDumping/General/1.0.0/Opc.Ua.Mining.TransportDumping.General.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "machinery" DEPENDS "mining-general")

    elseif(MODEL STREQUAL "Mining-TransportDumping-RearDumpTruck")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/Mining/TransportDumping/RearDumpTruck/1.0.0/NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/Mining/TransportDumping/RearDumpTruck/1.0.0/Opc.Ua.Mining.TransportDumping.RearDumpTruck.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "machinery" DEPENDS "mining-general"
                DEPENDS "mining-transportdumping-general")

    elseif(MODEL STREQUAL "Mining-TransportDumping-ArmouredFaceConveyor")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/Mining/TransportDumping/ArmouredFaceConveyor/1.0.0/NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/Mining/TransportDumping/ArmouredFaceConveyor/1.0.0/Opc.Ua.Mining.TransportDumping.ArmouredFaceConveyor.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "machinery" DEPENDS "mining-general"
                DEPENDS "mining-transportdumping-general")

    # Mining MineralProcessing
    elseif(MODEL STREQUAL "Mining-MineralProcessing-General")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/Mining/MineralProcessing/General/1.0.0/NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/Mining/MineralProcessing/General/1.0.0/Opc.Ua.Mining.MineralProcessing.General.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "machinery" DEPENDS "mining-general")

    elseif(MODEL STREQUAL "Mining-MineralProcessing-RockCrusher")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/Mining/MineralProcessing/RockCrusher/1.0.0/NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/Mining/MineralProcessing/RockCrusher/1.0.0/Opc.Ua.Mining.MineralProcessing.RockCrusher.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "machinery" DEPENDS "mining-general"
                DEPENDS "mining-mineralprocessing-general")

    # Mining PELOServices
    elseif(MODEL STREQUAL "Mining-PELOServices-General")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/Mining/PELOServices/General/1.0.0/NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/Mining/PELOServices/General/1.0.0/Opc.Ua.Mining.PELOServices.General.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "machinery" DEPENDS "mining-general")

    elseif(MODEL STREQUAL "Mining-PELOServices-FaceAlignmentSystem")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/Mining/PELOServices/FaceAlignmentSystem/1.0.0/NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/Mining/PELOServices/FaceAlignmentSystem/1.0.0/Opc.Ua.Mining.PELOServices.FaceAlignmentSystem.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "machinery" DEPENDS "mining-general"
                DEPENDS "mining-peloservices-general")

    # Mining MonitoringSupervisionServices
    elseif(MODEL STREQUAL "Mining-MonitoringSupervisionServices-General")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/Mining/MonitoringSupervisionServices/General/1.0.0/NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/Mining/MonitoringSupervisionServices/General/1.0.0/Opc.Ua.Mining.MonitoringSupervisionServices.General.NodeSet2.xml"
                INTERNAL AUTOLOAD
                DEPENDS "di" DEPENDS "machinery" DEPENDS "mining-general")

    else()
        MESSAGE(WARNING "The specified nodeset '${MODEL}' is not supported or is misspelled.")
    endif()
endforeach()

set(UA_NODESETINJECTOR_SOURCE_FILES ${UA_NODESETINJECTOR_SOURCE_FILES} PARENT_SCOPE)
set(UA_NODESETINJECTOR_NAMES ${UA_NODESETINJECTOR_NAMES} PARENT_SCOPE)
