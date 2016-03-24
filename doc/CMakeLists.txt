find_package(Sphinx REQUIRED)

make_directory(${PROJECT_BINARY_DIR}/doc_src)
file(GLOB DOC_SRC "${PROJECT_SOURCE_DIR}/doc/*")
file(COPY ${DOC_SRC} DESTINATION ${PROJECT_BINARY_DIR}/doc_src)

function(generate_rst in out)
  add_custom_command(OUTPUT ${out}
                     DEPENDS ${PROJECT_SOURCE_DIR}/tools/c2rst.py ${in}
                     PRE_BUILD
                     COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/c2rst.py ${in} ${out})
endfunction()

generate_rst(${PROJECT_SOURCE_DIR}/include/ua_types.h ${PROJECT_BINARY_DIR}/doc_src/types.rst)

add_custom_target(doc_latex ${SPHINX_EXECUTABLE}
  -b latex -c "${PROJECT_BINARY_DIR}/doc_src" "${PROJECT_BINARY_DIR}/doc_src" "${PROJECT_BINARY_DIR}/doc_latex"
  DEPENDS ${PROJECT_BINARY_DIR}/doc_src/types.rst
  COMMENT "Building LaTeX sources for documentation with Sphinx")
add_custom_target(doc ${SPHINX_EXECUTABLE}
  -b html -c "${PROJECT_BINARY_DIR}/doc_src" "${PROJECT_BINARY_DIR}/doc_src" "${PROJECT_BINARY_DIR}/doc"
  COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_BINARY_DIR}/doc_src/open62541_html.png" "${PROJECT_BINARY_DIR}/doc/_static/"
  DEPENDS ${PROJECT_BINARY_DIR}/doc_src/types.rst
  COMMENT "Building HTML documentation with Sphinx")

