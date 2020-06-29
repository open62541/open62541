include(CMakeParseArguments)

# Calls conan install to get the required dependnecies, specified in a conanfile.txt 
# If no conanfile.txt was provided tries to search for one in the local project directory
# Given conanfile name does not matter if it confirms with typical file naming
# @Author: Dovydas Girdvainis
# @Date: 2020-02-12
macro(execute_conan_install)
    set(options SILENT)
    set(oneValueArgs CONANFILE)
    set(multiValueArgs DUMMY)
    cmake_parse_arguments(EXECUTE_CONAN_INSTALL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    find_program(conan_command conan)  
    if(NOT conan_command)
      message(FATAL_ERROR "Conan executable not found! If conan is not installed follow the instructions at: https://docs.conan.io/en/latest/installation.html")
    else()
      message(STATUS "Found program: ${conan_command}")
      execute_process(COMMAND ${conan_command} --version
                      OUTPUT_VARIABLE CONAN_VERSION_OUTPUT
      )
      message(STATUS "Using Conan Version ${CONAN_VERSION_OUTPUT}")  
    endif()

    if("${CONANFILE}" STREQUAL "")
      find_file(LOCAL_CONANFILE conanfile.txt PATHS ${PROJECT_SOURCE_DIR} NO_DEFAULT_PATH)
      if(NOT EXISTS "${LOCAL_CONANFILE}") 
        message(FATAL_ERROR "No conanafile found in local project directory: ${PROJECT_SOURCE_DIR}")
      else()
        message(STATUS "Using local conanfile: ${LOCAL_CONANFILE}")
        set(CONANFILE ${LOCAL_CONANFILE})
      endif()
    else()
      if(NOT EXISTS "${CONANFILE}") 
        message(FATAL_ERROR "Provided conanafile: ${CONANFILE} does not exist!")
      endif()
    endif()

    execute_process(COMMAND ${conan_command} install ${CONANFILE}
                    RESULT_VARIABLE return_code
                    OUTPUT_VARIABLE conan_output
                    ERROR_VARIABLE conan_error
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    if(NOT SILENT)
      message("${conan_output}")
    endif()
    
    if(NOT "${return_code}" STREQUAL "0")
      message(WARNING "Conan failt to install failed!")
      message("${conan_error}")
      message(WARNING "Trying to build sources locally!")
      execute_process(COMMAND ${conan_command} install ${CONANFILE} --build
                      RESULT_VARIABLE return_code
                      OUTPUT_VARIABLE conan_output
                      ERROR_VARIABLE conan_error
                      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      )

      if(NOT SILENT)
        message("${conan_output}")
      endif()

      if(NOT "${return_code}" STREQUAL "0")
        message(FATAL_ERROR "${conan_error}")
      endif()
    endif()
    message(STATUS "Using generated conan paths from: ${CMAKE_BINARY_DIR}/conan_paths.cmake")
    include(${CMAKE_BINARY_DIR}/conan_paths.cmake)

    
endmacro(execute_conan_install)
