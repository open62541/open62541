#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "certificates.h"
#include "check.h"
#include "thread_wrapper.h"

UA_Server *server;
UA_Boolean running;
THREAD_HANDLE server_thread;

const size_t trustListSize = 0;
const UA_ByteString *const trustList = NULL;

const size_t revocationListSize = 0;
const UA_ByteString *const revocationList = NULL;

static void initialize_server_config(UA_ServerConfig *config) {
    const UA_ByteString certificate = {CERT_DER_LENGTH, CERT_DER_DATA};
    const UA_ByteString privateKey = {KEY_DER_LENGTH, KEY_DER_DATA};

    const size_t trustListSize = 1;
    const UA_ByteString trustList[1] = {{ROOT_CERT_DER_LENGTH, ROOT_CERT_DER_DATA}};

    const size_t issuerListSize = 1;
    const UA_ByteString issuerList[1] = {{INTERMEDIATE_CERT_DER_LENGTH, INTERMEDIATE_CERT_DER_DATA}};

    UA_ServerConfig_setDefaultWithSecurityPolicies(config, 4840, &certificate, &privateKey,
                                                   trustList, trustListSize,
                                                   issuerList, issuerListSize,
                                                   revocationList, revocationListSize);

    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri =
        UA_STRING_ALLOC("urn:unconfigured:application");
}

static UA_Server *create_server(void) {
    static UA_ServerConfig config;
    memset(&config, 0, sizeof(config));
    initialize_server_config(&config);

    /* move config into the server */
    UA_Server *server = UA_Server_newWithConfig(&config);

    /* config should no longer be used;
     * if it was a stack variable, it would be destroyed on return;
     * the following assignment should have no effect;
     * however, it causes a crash in v1.4.16 */
    config.secureChannelPKI.logging = (UA_Logger *)1;

    return server;
}

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void setup(void) {
    server = create_server();
    ck_assert(server != NULL);

    running = true;
    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(encryption_connect) {
    const UA_ByteString certificate = {APPLICATION_CERT_DER_LENGTH, APPLICATION_CERT_DER_DATA};
    const UA_ByteString privateKey = {APPLICATION_KEY_DER_LENGTH, APPLICATION_KEY_DER_DATA};

    UA_Client *client = UA_Client_new();
    ck_assert(client != NULL);

    UA_ClientConfig *config = UA_Client_getConfig(client);
    UA_ClientConfig_setDefaultEncryption(config, certificate, privateKey,
                                         trustList, trustListSize,
                                         revocationList, revocationListSize);

    UA_String_clear(&config->clientDescription.applicationUri);
    config->clientDescription.applicationUri =
        UA_STRING_ALLOC("urn:unconfigured:application");

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

static Suite* testSuite_encryption(void) {
    Suite *s = suite_create("Encryption");
    TCase *tc_encryption = tcase_create("Encryption basic256sha256 (copy config)");
    tcase_add_checked_fixture(tc_encryption, setup, teardown);
#ifdef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc_encryption, encryption_connect);
#endif /* UA_ENABLE_ENCRYPTION */
    suite_add_tcase(s,tc_encryption);
    return s;
}

int main(void) {
    Suite *s = testSuite_encryption();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
