#include <stdlib.h>
#include <stdio.h>
#include "check.h"

#include "ua_server.h"
#include "ua_server_internal.h"
#include "ua_config_standard.h"
#include "ua_log_stdout.h"
#include "testing_networklayers.h"

size_t files;
char **filenames;

static UA_ByteString readFile(char *filename) {
    UA_ByteString buf = UA_BYTESTRING_NULL;
    size_t length;
    FILE *f = fopen(filename,"r");

    if(f) {
        fseek(f, 0, SEEK_END);
        length = ftell(f);
        rewind(f);
        buf.data = malloc(length);
        fread(buf.data, sizeof(char), length, f);
        buf.length = length;
        fclose(f);
    }

    return buf;
}

START_TEST(processMessage) {
    UA_Connection c = createDummyConnection();
    UA_ServerConfig config = UA_ServerConfig_standard;
    config.logger = UA_Log_Stdout;
    UA_Server *server = UA_Server_new(config);
    for(size_t i = 0; i < files; i++) {
        UA_ByteString msg = readFile(filenames[i]);
        UA_Boolean reallocated;
        UA_StatusCode retval = UA_Connection_completeMessages(&c, &msg, &reallocated);
        if(retval == UA_STATUSCODE_GOOD && msg.length > 0)
            UA_Server_processBinaryMessage(server, &c, &msg);
        UA_ByteString_deleteMembers(&msg);
    }
    UA_Server_delete(server);
    UA_Connection_deleteMembers(&c);
}
END_TEST

static Suite *testSuite_binaryMessages(void) {
    Suite *s = suite_create("Test server with messages stored in text files");
    TCase *tc_messages = tcase_create("binary messages");
    tcase_add_test(tc_messages, processMessage);
    suite_add_tcase(s, tc_messages);
    return s;
}

int main(int argc, char **argv) {
    if(argc < 2)
        return EXIT_FAILURE;
    filenames = &argv[1];
    files = argc - 1;
    int number_failed = 0;
    Suite *s;
    SRunner *sr;
    s  = testSuite_binaryMessages();
    sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
