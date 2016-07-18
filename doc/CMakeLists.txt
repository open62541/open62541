find_package(Sphinx REQUIRED)
find_package(LATEX)

set(DOC_LATEX_DIR ${PROJECT_BINARY_DIR}/doc_latex)
set(DOC_HTML_DIR  ${PROJECT_BINARY_DIR}/doc)
set(DOC_SRC_DIR   ${PROJECT_BINARY_DIR}/doc_src)

make_directory(${DOC_SRC_DIR})
file(GLOB DOC_SRC "${PROJECT_SOURCE_DIR}/doc/*")
file(COPY ${DOC_SRC} DESTINATION ${DOC_SRC_DIR})

function(generate_rst in out)
  add_custom_command(OUTPUT ${out} DEPENDS ${PROJECT_SOURCE_DIR}/tools/c2rst.py ${in}
                     PRE_BUILD COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/c2rst.py ${in} ${out})
endfunction()

generate_rst(${PROJECT_SOURCE_DIR}/include/ua_types.h ${DOC_SRC_DIR}/types.rst)
generate_rst(${PROJECT_SOURCE_DIR}/include/ua_constants.h ${DOC_SRC_DIR}/constants.rst)
generate_rst(${PROJECT_BINARY_DIR}/src_generated/ua_types_generated.h ${DOC_SRC_DIR}/types_generated.rst)
generate_rst(${PROJECT_SOURCE_DIR}/include/ua_server.h ${DOC_SRC_DIR}/server.rst)
generate_rst(${PROJECT_SOURCE_DIR}/include/ua_client.h ${DOC_SRC_DIR}/client.rst)
generate_rst(${PROJECT_SOURCE_DIR}/include/ua_client_highlevel.h ${DOC_SRC_DIR}/client_highlevel.rst)
generate_rst(${PROJECT_SOURCE_DIR}/include/ua_log.h ${DOC_SRC_DIR}/log.rst)
generate_rst(${PROJECT_SOURCE_DIR}/include/ua_connection.h ${DOC_SRC_DIR}/connection.rst)
generate_rst(${PROJECT_SOURCE_DIR}/src/server/ua_services.h ${DOC_SRC_DIR}/services.rst)
generate_rst(${PROJECT_SOURCE_DIR}/src/server/ua_nodestore.h ${DOC_SRC_DIR}/nodestore.rst)

add_custom_target(doc_latex ${SPHINX_EXECUTABLE}
  -b latex "${DOC_SRC_DIR}" "${DOC_LATEX_DIR}"
  DEPENDS ${DOC_SRC_DIR}/types.rst ${DOC_SRC_DIR}/constants.rst ${DOC_SRC_DIR}/types_generated.rst
          ${DOC_SRC_DIR}/server.rst ${DOC_SRC_DIR}/client.rst ${DOC_SRC_DIR}/client_highlevel.rst
          ${DOC_SRC_DIR}/log.rst ${DOC_SRC_DIR}/connection.rst ${DOC_SRC_DIR}/services.rst ${DOC_SRC_DIR}/nodestore.rst
  COMMENT "Building LaTeX sources for documentation with Sphinx")
add_dependencies(doc_latex open62541)


add_custom_target(doc_pdf ${PDFLATEX_COMPILER} -q "open62541.tex"
  WORKING_DIRECTORY ${DOC_LATEX_DIR}
  # compile it twice so that the contents pages are correct
  COMMAND ${PDFLATEX_COMPILER} -q "open62541.tex"
  DEPENDS ${PDFLATEX_COMPILER}
  COMMENT "Generating PDF documentation from LaTeX sources")
add_dependencies(doc_pdf doc_latex)

add_custom_target(doc ${SPHINX_EXECUTABLE}
  -b html "${DOC_SRC_DIR}" "${DOC_HTML_DIR}"
  DEPENDS ${DOC_SRC_DIR}/types.rst ${DOC_SRC_DIR}/constants.rst ${DOC_SRC_DIR}/types_generated.rst
          ${DOC_SRC_DIR}/server.rst ${DOC_SRC_DIR}/client.rst ${DOC_SRC_DIR}/client_highlevel.rst
          ${DOC_SRC_DIR}/log.rst ${DOC_SRC_DIR}/connection.rst ${DOC_SRC_DIR}/services.rst ${DOC_SRC_DIR}/nodestore.rst
  COMMENT "Building HTML documentation with Sphinx")
add_dependencies(doc open62541)
