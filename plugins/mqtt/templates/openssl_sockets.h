#if !defined(__OPENSSL_SOCKET_TEMPLATE_H__)
#define __OPENSSL_SOCKET_TEMPLATE_H__

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <string.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/util.h>

/*
    A template for opening a non-blocking OpenSSL connection.
*/
UA_StatusCode open_nb_socket(BIO**       bio,
                    SSL_CTX**   ssl_ctx,
                    const char* addr,
                    const char* port,
                    const char* ca_file,
                    const char* ca_path,
                    const char* cert_file,
                    const char* key_file);

UA_StatusCode open_nb_socket(BIO**       bio,
                    SSL_CTX**   ssl_ctx,
                    const char* addr,
                    const char* port,
                    const char* ca_file,
                    const char* ca_path,
                    const char* cert_file,
                    const char* key_file)
{
    *ssl_ctx = SSL_CTX_new(SSLv23_client_method());
    SSL* ssl;

    /* load certificate */
    if (!SSL_CTX_load_verify_locations(*ssl_ctx, ca_file, ca_path)) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "MQTT PubSub: Failed to load ca certificate");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    if (cert_file && key_file)
    {
        if (!SSL_CTX_use_certificate_file(*ssl_ctx, cert_file, SSL_FILETYPE_PEM))
        {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                         "MQTT PubSub: Failed to load client certificate");
            return UA_STATUSCODE_BADINTERNALERROR;
        }

        if (!SSL_CTX_use_PrivateKey_file(*ssl_ctx, key_file, SSL_FILETYPE_PEM))
        {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                         "MQTT PubSub: Failed to load client key");
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    }

    /* open BIO socket */
    char * addr_copy = (char*)malloc(strlen(addr) + 1);
    strcpy(addr_copy,addr);
    char * port_copy = (char*)malloc(strlen(port) + 1);
    strcpy(port_copy,port);

    *bio = BIO_new_ssl_connect(*ssl_ctx);
    BIO_get_ssl(*bio, &ssl);
    SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
    BIO_set_conn_hostname(*bio, addr_copy);
    BIO_set_nbio(*bio, 1);
    BIO_set_conn_port(*bio, port_copy);

    free(addr_copy);
    free(port_copy);

    /* wait for connect with 10 second timeout */
    int start_time = (int)time(NULL);
    int do_connect_rv = (int)BIO_do_connect(*bio);
    while(do_connect_rv <= 0 && BIO_should_retry(*bio) && (int)time(NULL) - start_time < 10) {
        do_connect_rv = (int)BIO_do_connect(*bio);
    }
    if (do_connect_rv <= 0) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "MQTT PubSub: Failed to open socket: %s", ERR_reason_error_string(ERR_get_error()));
        BIO_free_all(*bio);
        SSL_CTX_free(*ssl_ctx);
        *bio = NULL;
        *ssl_ctx=NULL;
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* verify certificate */
    if (SSL_get_verify_result(ssl) != X509_V_OK) {
        /* Handle the failed verification */
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "MQTT PubSub: x509 certificate verification failed");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

#endif
