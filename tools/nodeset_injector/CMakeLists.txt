foreach(MODEL ${UA_INFORMATION_MODEL_AUTOLOAD})
    string(TOLOWER "${MODEL}" NODESET_NAME)

    # only one xml file
    # no dependency
    if(MODEL STREQUAL "DEXPI" OR MODEL STREQUAL "Safety" OR MODEL STREQUAL "CNC" OR MODEL STREQUAL "BACnet"
       OR MODEL STREQUAL "AMB" OR MODEL STREQUAL "MachineVision" OR MODEL STREQUAL "RSL" OR MODEL STREQUAL "I4AAS"
       OR MODEL STREQUAL "PackML" OR MODEL STREQUAL "MTConnect")
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

        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${csv_file}"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${xml_file}"
                INTERNAL
                AUTOLOAD
        )

    # only one xml file
    # di as dependency
    elseif(MODEL STREQUAL "CSPPlusForMachine" OR MODEL STREQUAL "POWERLINK"
       OR MODEL STREQUAL "IA" OR MODEL STREQUAL "FDT" OR MODEL STREQUAL "Robotics"
       OR MODEL STREQUAL "CommercialKitchenEquipment" OR MODEL STREQUAL "Machinery")
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

        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${csv_file}"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${xml_file}"
                INTERNAL
                AUTOLOAD
                DEPENDS "di"
        )

    # more dependencies
    elseif(MODEL STREQUAL "CAS")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeSet2.xml"
                INTERNAL
                AUTOLOAD
                DEPENDS "di"
                DEPENDS "machinery"
                DEPENDS "ia"
        )
    elseif(MODEL STREQUAL "TMC")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeSet2.xml"
                INTERNAL
                AUTOLOAD
                DEPENDS "di"
                DEPENDS "packml"
        )
    elseif(MODEL STREQUAL "Pumps")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeSet2.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeSet2.xml"
                INTERNAL
                AUTOLOAD
                DEPENDS "di"
                DEPENDS "machinery"
        )
    elseif(MODEL STREQUAL "Weihenstephan")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeSet2.xml"
                INTERNAL
                AUTOLOAD
                DEPENDS "di"
                DEPENDS "machinery"
                DEPENDS "packml"
        )
    elseif(MODEL STREQUAL "MachineTool")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeSet2.xml"
                INTERNAL
                AUTOLOAD
                DEPENDS "di"
                DEPENDS "machinery"
                DEPENDS "ia"
        )
        elseif(MODEL STREQUAL "Scales")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeSet2.xml"
                INTERNAL
                AUTOLOAD
                DEPENDS "di"
                DEPENDS "machinery"
                DEPENDS "packml"
        )
    # Extra
    elseif(MODEL STREQUAL "DI")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.Di.NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.Di.NodeSet2.xml"
                INTERNAL
                AUTOLOAD
        )
    elseif(MODEL STREQUAL "AutoID")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${PROJECT_SOURCE_DIR}/tools/nodeset_injector/schema/Opc.Ua.AutoID.NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.AutoID.NodeSet2.xml"
                INTERNAL
                AUTOLOAD
                DEPENDS "di"
        )
    elseif(MODEL STREQUAL "PROFINET")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.Pn.NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.Pn.NodeSet2.xml"
                INTERNAL
                AUTOLOAD
        )
    elseif(MODEL STREQUAL "Glass")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Flat/NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Flat/Opc.Ua.${MODEL}.NodeSet2.xml"
                INTERNAL
                AUTOLOAD
                DEPENDS "di"
                DEPENDS "machinery"
        )
    elseif(MODEL STREQUAL "IJT")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Tightening/NodeIds.csv"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Tightening/Opc.Ua.Ijt.Tightening.NodeSet2.xml"
                INTERNAL
                AUTOLOAD
                DEPENDS "di"
                DEPENDS "machinery"
        )
    elseif(MODEL STREQUAL "OpenSCS")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.OPENSCS.NodeSet2.xml"
                INTERNAL
                AUTOLOAD
        )
    elseif(MODEL STREQUAL "PNEM")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.PnEm.NodeSet2.xml"
                INTERNAL
                AUTOLOAD
                DEPENDS "di"
        )
    elseif(MODEL STREQUAL "PNRIO")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/NodeIds.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.PnRio.NodeSet2.xml"
                INTERNAL
                AUTOLOAD
                DEPENDS "di"
        )
    elseif(MODEL STREQUAL "ISA95-JOBCONTROL")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/opc.ua.isa95-jobcontrol.nodeset2.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/opc.ua.isa95-jobcontrol.nodeset2.xml"
                INTERNAL
                AUTOLOAD
        )
    elseif(MODEL STREQUAL "PLCopen")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeSet2_V1.02.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeSet2_V1.02.xml"
                INTERNAL
                AUTOLOAD
                DEPENDS "di"
        )
    elseif(MODEL STREQUAL "ADI")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/OpcUaAdiModel.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.Adi.NodeSet2.xml"
                INTERNAL
                AUTOLOAD
                DEPENDS "di"
        )
    elseif(MODEL STREQUAL "MDIS")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/MDIS.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.MDIS.NodeSet2.xml"
                INTERNAL
                AUTOLOAD
        )
    elseif(MODEL STREQUAL "Sercos")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/SercosModel.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Sercos.NodeSet2.xml"
                INTERNAL
                AUTOLOAD
                DEPENDS "di"
        )
    elseif(MODEL STREQUAL "GDS")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/OpcUaGdsModel.csv"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.Gds.NodeSet2.xml"
                INTERNAL
                AUTOLOAD
        )
    elseif(MODEL STREQUAL "PADIM")
        ua_generate_nodeset_and_datatypes(
                NAME "irdi"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.IRDI.NodeSet2.xml"
                INTERNAL
                AUTOLOAD
        )

        set(TMP_NSURI "http://opcfoundation.org/UA/${MODEL}/")
        configure_file(${PROJECT_SOURCE_DIR}/tools/nodeset_injector/empty.bsd.template "${CMAKE_BINARY_DIR}/bsd_files_gen/Opc.Ua.${MODEL}.Types.bsd")
        ua_generate_nodeset_and_datatypes(
                NAME "${NODESET_NAME}"
                FILE_NS "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeSet2.xml"
                FILE_CSV "${UA_NODESET_DIR}/${MODEL}/Opc.Ua.${MODEL}.NodeIds.csv"
                FILE_BSD "${CMAKE_BINARY_DIR}/bsd_files_gen/Opc.Ua.${MODEL}.Types.bsd"
                INTERNAL
                AUTOLOAD
                DEPENDS "di"
                DEPENDS "irdi"
        )
    else()
        MESSAGE(WARNING "The specified nodeset '${MODEL}' is not supported or is misspelled.")
    endif()

endforeach()


# TODO: remove when CMP0118 is NEW (cmake 3.20)
set_source_files_properties(${PROJECT_BINARY_DIR}/src_generated/open62541/nodesetinjector.c ${PROJECT_BINARY_DIR}/src_generated/open62541/nodesetinjector.h PROPERTIES GENERATED 1)
add_library(autoinject ${UA_NODESETINJECTOR_SOURCE_FILES} ${PROJECT_BINARY_DIR}/src_generated/open62541/nodesetinjector.c)
set_target_properties(autoinject PROPERTIES PUBLIC_HEADER "${UA_NODESETINJECTOR_HEADER_FILES}")
target_link_libraries(autoinject open62541)

# Create alias so that it can be found under open62541::autoinject
add_library(open62541::autoinject ALIAS autoinject)

# Define installation for the library
install(TARGETS autoinject
        EXPORT open62541Targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/autoinject
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT open62541Targets
        FILE open62541Targets.cmake
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/open62541"
        NAMESPACE open62541::)

include(CMakePackageConfigHelpers)
configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/autoinjectConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/autoinjectConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/open62541
)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/autoinjectConfig.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/open62541
)

foreach(EXAMPLE_NAME ${UA_NODESETINJECTOR_EXAMPLE_NAMES})
    if(UA_NODESETINJECTOR_GENERATORS)
        target_link_libraries(${EXAMPLE_NAME} autoinject)
        add_dependencies(${EXAMPLE_NAME} ${UA_NODESETINJECTOR_GENERATORS})
    endif()
endforeach()

if(UA_BUILD_UNIT_TESTS)
    foreach(TEST_NAME ${UA_NODESETINJECTOR_TEST_NAMES})
        if(UA_NODESETINJECTOR_GENERATORS)
            target_link_libraries(${TEST_NAME} autoinject)
            add_dependencies(${TEST_NAME} ${UA_NODESETINJECTOR_GENERATORS})
        endif()
    endforeach()
endif()
