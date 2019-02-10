find_package(Sphinx)
find_package(LATEX)

set(DOC_LATEX_DIR ${PROJECT_BINARY_DIR}/doc_latex)
set(DOC_HTML_DIR  ${PROJECT_BINARY_DIR}/doc)
set(DOC_SRC_DIR   ${PROJECT_BINARY_DIR}/doc_src)

make_directory(${DOC_SRC_DIR})
file(GLOB DOC_SRC "${PROJECT_SOURCE_DIR}/doc/*")
list(REMOVE_ITEM DOC_SRC "${PROJECT_SOURCE_DIR}/doc/conf.py")
file(COPY ${DOC_SRC} DESTINATION ${DOC_SRC_DIR})
configure_file("${PROJECT_SOURCE_DIR}/doc/conf.py" "${DOC_SRC_DIR}/conf.py")

function(generate_rst in out)
  add_custom_command(OUTPUT ${out} DEPENDS ${PROJECT_SOURCE_DIR}/tools/c2rst.py ${in}
                     PRE_BUILD COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/c2rst.py ${in} ${out})
endfunction()

# Doc from headers
generate_rst(${PROJECT_SOURCE_DIR}/include/open62541/types.h ${DOC_SRC_DIR}/types.rst)
generate_rst(${PROJECT_SOURCE_DIR}/include/open62541/constants.h ${DOC_SRC_DIR}/constants.rst)
generate_rst(${PROJECT_BINARY_DIR}/src_generated/open62541/statuscodes.h ${DOC_SRC_DIR}/statuscodes.rst)
generate_rst(${PROJECT_BINARY_DIR}/src_generated/open62541/types_generated.h ${DOC_SRC_DIR}/types_generated.rst)
generate_rst(${PROJECT_SOURCE_DIR}/include/open62541/server_config.h ${DOC_SRC_DIR}/server_config.rst)
generate_rst(${PROJECT_SOURCE_DIR}/include/open62541/server.h ${DOC_SRC_DIR}/server.rst)
generate_rst(${PROJECT_SOURCE_DIR}/include/open62541/client_config.h ${DOC_SRC_DIR}/client_config.rst)
generate_rst(${PROJECT_SOURCE_DIR}/include/open62541/client_subscriptions.h ${DOC_SRC_DIR}/client_subscriptions.rst)
generate_rst(${PROJECT_SOURCE_DIR}/include/open62541/client.h ${DOC_SRC_DIR}/client.rst)
generate_rst(${PROJECT_SOURCE_DIR}/include/open62541/client_highlevel.h ${DOC_SRC_DIR}/client_highlevel.rst)
generate_rst(${PROJECT_SOURCE_DIR}/include/open62541/plugin/log.h ${DOC_SRC_DIR}/plugin_log.rst)
generate_rst(${PROJECT_SOURCE_DIR}/include/open62541/plugin/network.h ${DOC_SRC_DIR}/plugin_network.rst)
generate_rst(${PROJECT_SOURCE_DIR}/include/open62541/plugin/accesscontrol.h ${DOC_SRC_DIR}/plugin_accesscontrol.rst)
generate_rst(${PROJECT_SOURCE_DIR}/src/server/ua_services.h ${DOC_SRC_DIR}/services.rst)
generate_rst(${PROJECT_SOURCE_DIR}/include/open62541/plugin/nodestore.h ${DOC_SRC_DIR}/nodestore.rst)
generate_rst(${PROJECT_SOURCE_DIR}/include/open62541/plugin/pubsub.h ${DOC_SRC_DIR}/plugin_pubsub_connection.rst)
generate_rst(${PROJECT_SOURCE_DIR}/include/open62541/server_pubsub.h ${DOC_SRC_DIR}/pubsub.rst)

# Doc from tutorial code
generate_rst(${PROJECT_SOURCE_DIR}/examples/tutorial_datatypes.c ${DOC_SRC_DIR}/tutorial_datatypes.rst)
generate_rst(${PROJECT_SOURCE_DIR}/examples/tutorial_server_firststeps.c ${DOC_SRC_DIR}/tutorial_server_firststeps.rst)
generate_rst(${PROJECT_SOURCE_DIR}/examples/tutorial_server_variable.c ${DOC_SRC_DIR}/tutorial_server_variable.rst)
generate_rst(${PROJECT_SOURCE_DIR}/examples/tutorial_server_variabletype.c ${DOC_SRC_DIR}/tutorial_server_variabletype.rst)
generate_rst(${PROJECT_SOURCE_DIR}/examples/tutorial_server_datasource.c ${DOC_SRC_DIR}/tutorial_server_datasource.rst)
generate_rst(${PROJECT_SOURCE_DIR}/examples/tutorial_server_events.c ${DOC_SRC_DIR}/tutorial_server_events.rst)
generate_rst(${PROJECT_SOURCE_DIR}/examples/tutorial_server_monitoreditems.c ${DOC_SRC_DIR}/tutorial_server_monitoreditems.rst)
generate_rst(${PROJECT_SOURCE_DIR}/examples/tutorial_server_object.c ${DOC_SRC_DIR}/tutorial_server_object.rst)
generate_rst(${PROJECT_SOURCE_DIR}/examples/tutorial_server_method.c ${DOC_SRC_DIR}/tutorial_server_method.rst)
generate_rst(${PROJECT_SOURCE_DIR}/examples/tutorial_client_firststeps.c ${DOC_SRC_DIR}/tutorial_client_firststeps.rst)
generate_rst(${PROJECT_SOURCE_DIR}/examples/pubsub/tutorial_pubsub_publish.c ${DOC_SRC_DIR}/tutorial_pubsub_publish.rst)

add_custom_target(doc_latex ${SPHINX_EXECUTABLE}
  -b latex "${DOC_SRC_DIR}" "${DOC_LATEX_DIR}"
  DEPENDS ${DOC_SRC_DIR}/types.rst ${DOC_SRC_DIR}/constants.rst ${DOC_SRC_DIR}/types_generated.rst
          ${DOC_SRC_DIR}/statuscodes.rst
          ${DOC_SRC_DIR}/server.rst ${DOC_SRC_DIR}/server_config.rst
          ${DOC_SRC_DIR}/client.rst ${DOC_SRC_DIR}/client_subscriptions.rst
          ${DOC_SRC_DIR}/client_highlevel.rst ${DOC_SRC_DIR}/client_config.rst
          ${DOC_SRC_DIR}/plugin_log.rst ${DOC_SRC_DIR}/plugin_network.rst
          ${DOC_SRC_DIR}/services.rst ${DOC_SRC_DIR}/plugin_accesscontrol.rst
          ${DOC_SRC_DIR}/nodestore.rst
          ${DOC_SRC_DIR}/tutorials.rst
          ${DOC_SRC_DIR}/tutorial_datatypes.rst
          ${DOC_SRC_DIR}/tutorial_client_firststeps.rst
          ${DOC_SRC_DIR}/tutorial_server_firststeps.rst
          ${DOC_SRC_DIR}/tutorial_server_variable.rst
          ${DOC_SRC_DIR}/tutorial_server_variabletype.rst
          ${DOC_SRC_DIR}/tutorial_server_datasource.rst
          ${DOC_SRC_DIR}/tutorial_server_monitoreditems.rst
          ${DOC_SRC_DIR}/tutorial_server_object.rst
          ${DOC_SRC_DIR}/tutorial_server_method.rst
          ${DOC_SRC_DIR}/tutorial_pubsub_publish.rst
          ${DOC_SRC_DIR}/plugin_pubsub_connection.rst
          ${DOC_SRC_DIR}/pubsub.rst
          ${DOC_SRC_DIR}/tutorial_server_events.rst
  COMMENT "Building LaTeX sources for documentation with Sphinx")
add_dependencies(doc_latex open62541)

add_custom_target(doc_pdf ${PDFLATEX_COMPILER} -interaction=batchmode "open62541.tex"
  WORKING_DIRECTORY ${DOC_LATEX_DIR}
  # compile it twice so that the contents pages are correct
  COMMAND ${PDFLATEX_COMPILER} -interaction=batchmode "open62541.tex"
  DEPENDS ${PDFLATEX_COMPILER}
  COMMENT "Generating PDF documentation from LaTeX sources")
add_dependencies(doc_pdf doc_latex)

add_custom_target(doc ${SPHINX_EXECUTABLE}
  -b html "${DOC_SRC_DIR}" "${DOC_HTML_DIR}"
  DEPENDS ${DOC_SRC_DIR}/types.rst ${DOC_SRC_DIR}/constants.rst ${DOC_SRC_DIR}/types_generated.rst
          ${DOC_SRC_DIR}/statuscodes.rst
          ${DOC_SRC_DIR}/server.rst ${DOC_SRC_DIR}/server_config.rst
          ${DOC_SRC_DIR}/client.rst ${DOC_SRC_DIR}/client_subscriptions.rst
          ${DOC_SRC_DIR}/client_highlevel.rst ${DOC_SRC_DIR}/client_config.rst
          ${DOC_SRC_DIR}/plugin_log.rst ${DOC_SRC_DIR}/plugin_network.rst
          ${DOC_SRC_DIR}/services.rst ${DOC_SRC_DIR}/plugin_accesscontrol.rst
          ${DOC_SRC_DIR}/nodestore.rst
          ${DOC_SRC_DIR}/tutorials.rst
          ${DOC_SRC_DIR}/tutorial_datatypes.rst
          ${DOC_SRC_DIR}/tutorial_client_firststeps.rst
          ${DOC_SRC_DIR}/tutorial_server_firststeps.rst
          ${DOC_SRC_DIR}/tutorial_server_variable.rst
          ${DOC_SRC_DIR}/tutorial_server_variabletype.rst
          ${DOC_SRC_DIR}/tutorial_server_datasource.rst
          ${DOC_SRC_DIR}/tutorial_server_monitoreditems.rst
          ${DOC_SRC_DIR}/tutorial_server_object.rst
          ${DOC_SRC_DIR}/tutorial_server_method.rst
          ${DOC_SRC_DIR}/tutorial_pubsub_publish.rst
          ${DOC_SRC_DIR}/plugin_pubsub_connection.rst
          ${DOC_SRC_DIR}/pubsub.rst
          ${DOC_SRC_DIR}/tutorial_server_events.rst
  COMMENT "Building HTML documentation with Sphinx")
add_dependencies(doc open62541)

set_target_properties(doc doc_latex doc_pdf PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD TRUE)
set_target_properties(doc doc_latex doc_pdf PROPERTIES FOLDER "open62541/doc")
