set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(cert_encrypt_tpm EXCLUDE_FROM_ALL cert_encrypt_tpm.c)
target_link_libraries(cert_encrypt_tpm tpm2_pkcs11 ssl crypto)
