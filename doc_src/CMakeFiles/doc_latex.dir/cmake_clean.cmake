file(REMOVE_RECURSE
  "CMakeFiles/doc_latex"
  "../doc_src/types.rst"
  "../doc_src/constants.rst"
  "../doc_src/types_generated.rst"
  "../doc_src/server.rst"
  "../doc_src/client.rst"
  "../doc_src/client_highlevel.rst"
  "../doc_src/plugin_log.rst"
  "../doc_src/plugin_network.rst"
  "../doc_src/services.rst"
  "../doc_src/plugin_access_control.rst"
  "../doc_src/nodestore.rst"
  "../doc_src/tutorial_datatypes.rst"
  "../doc_src/tutorial_client_firststeps.rst"
  "../doc_src/tutorial_server_firststeps.rst"
  "../doc_src/tutorial_server_variable.rst"
  "../doc_src/tutorial_server_variabletype.rst"
  "../doc_src/tutorial_server_datasource.rst"
  "../doc_src/tutorial_server_object.rst"
  "../doc_src/tutorial_server_method.rst"
)

# Per-language clean rules from dependency scanning.
foreach(lang)
  include(CMakeFiles/doc_latex.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
